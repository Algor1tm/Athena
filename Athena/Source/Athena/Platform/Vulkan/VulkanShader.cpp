#include "VulkanShader.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Time.h"
#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/VulkanRenderer.h"
#include "Athena/Platform/Vulkan/VulkanDebug.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"

#if defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif


namespace Athena
{
	namespace Utils
	{
		static FilePath GetCachedFilePath(const FilePath& path, ShaderType type)
		{
			FilePath relativeToShaderPack = std::filesystem::relative(path, Renderer::GetShaderPackDirectory());
			FilePath cachedPath = Renderer::GetShaderCacheDirectory() / relativeToShaderPack;

			switch (type)
			{
			case ShaderType::VERTEX_SHADER:    cachedPath += L".cached.vert"; break;
			case ShaderType::FRAGMENT_SHADER:  cachedPath += L".cached.frag"; break;
			case ShaderType::GEOMETRY_SHADER:  cachedPath += L".cached.geom"; break;
			case ShaderType::COMPUTE_SHADER:   cachedPath += L".cached.compute"; break;
			default: ATN_CORE_ASSERT(false); return "";
			}

			return cachedPath;
		}

		static shaderc_shader_kind ShaderTypeToShaderCKind(ShaderType type)
		{
			switch (type)
			{
			case ShaderType::VERTEX_SHADER:    return shaderc_glsl_vertex_shader;
			case ShaderType::FRAGMENT_SHADER:  return shaderc_glsl_fragment_shader;
			case ShaderType::GEOMETRY_SHADER:  return shaderc_glsl_geometry_shader;
			case ShaderType::COMPUTE_SHADER:   return shaderc_glsl_compute_shader;
			}

			ATN_CORE_ASSERT(false);
			return (shaderc_shader_kind)0;
		}


		static std::string_view ShaderTypeToString(ShaderType type)
		{
			switch (type)
			{
			case ShaderType::VERTEX_SHADER: return "Vertex Shader";
			case ShaderType::FRAGMENT_SHADER: return "Fragment Shader";
			case ShaderType::GEOMETRY_SHADER: return "Geometry Shader";
			case ShaderType::COMPUTE_SHADER: return "Compute Shader";
			}

			ATN_CORE_ASSERT(false);
			return "";
		}

		static bool ShaderTypeFromString(const String& strType, ShaderType& type)
		{
			if (strType == "VERTEX_STAGE")
				type = ShaderType::VERTEX_SHADER;
			else if (strType == "FRAGMENT_STAGE" || strType == "PIXEL_STAGE")
				type = ShaderType::FRAGMENT_SHADER;
			else if (strType == "GEOMETRY_STAGE")
				type = ShaderType::GEOMETRY_SHADER;
			else if (strType == "COMPUTE_STAGE")
				type = ShaderType::COMPUTE_SHADER;
			else
				return false;

			return true;
		}

		static std::string_view GetEntryPointName(ShaderType type)
		{
			switch (type)
			{
			case ShaderType::VERTEX_SHADER: return "VSMain";
			case ShaderType::FRAGMENT_SHADER: return "FSMain";
			case ShaderType::GEOMETRY_SHADER: return "GSMain";
			case ShaderType::COMPUTE_SHADER: return "CSMain";
			}

			ATN_CORE_ASSERT(false);
			return "";
		}
	}


	VulkanShader::VulkanShader(const FilePath& path, const String& name)
	{
		m_FilePath = path;
		m_Name = name;

		FilePath ext = m_FilePath.extension();
		m_IsHlsl = ext == L".hlsl" || ext == L".hlsli";

		CompileOrGetBinaries(false);
		if (m_Compiled)
			CreateVulkanShaderModulesAndStages();

		m_SPIRVBinaries.clear();
	}

	VulkanShader::VulkanShader(const FilePath& path)
		: VulkanShader(path, path.stem().string())
	{

	}

	VulkanShader::~VulkanShader()
	{
		CleanUp();
	}

	bool VulkanShader::IsCompiled()
	{
		return m_Compiled;
	}

	void VulkanShader::Reload()
	{
		CleanUp();

		CompileOrGetBinaries(true);
		if (m_Compiled)
			CreateVulkanShaderModulesAndStages();

		m_SPIRVBinaries.clear();
	}

	bool VulkanShader::PreProcess(const String& source, std::unordered_map<ShaderType, String>& result)
	{
		const char* hlslSettings = "#pragma pack_matrix( row_major )\n\n";

		const char* stageToken = "#pragma";
		uint64 stageTokenLength = strlen(stageToken);
		uint64 pos = source.find(stageToken, 0);
		while (pos != String::npos)
		{
			uint64 eol = source.find_first_of("\r\n", pos);

			if (eol == String::npos)
			{
				ATN_CORE_ERROR_TAG("Vulkan", "Failed to parse shader '{}'!", m_FilePath);
				return false;
			}

			uint64 begin = pos + stageTokenLength + 1;
			String typeString = source.substr(begin, eol - begin);

			ShaderType type;
			bool convertResult = Utils::ShaderTypeFromString(typeString, type);
			if (!convertResult)
			{
				ATN_CORE_ERROR_TAG("Vulkan", "Failed to parse shader '{}'!\n Error: invalid shader stage name.", m_Name);
				return false;
			}

			uint64 nextLinePos = source.find_first_not_of("\r,\n", eol);
			pos = source.find(stageToken, nextLinePos);
			String shaderSource = source.substr(nextLinePos, pos - (nextLinePos == String::npos ? source.size() - 1 : nextLinePos));

			if (m_IsHlsl)
				shaderSource.insert(0, hlslSettings);

			result[type] = shaderSource;
		}

		return true;
	}

	bool VulkanShader::CompileOrGetBinaries(bool forceCompile)
	{
		Timer compilationTimer;

		m_Compiled = false;
		if (!FileSystem::Exists(m_FilePath))
		{
			ATN_CORE_ERROR_TAG("Vulkan", "Invalid filepath for shader {}!", m_FilePath);
			return false;
		}

		FilePath ext = m_FilePath.extension();
		if (ext != L".hlsl" && ext != L".hlsli" && ext != L".glsl" && ext != L".glsli")
		{
			ATN_CORE_ERROR_TAG("Vulkan", "Invalid shader extension '{}'!", m_FilePath);
			return false;
		}

		String shaderString = FileSystem::ReadFile(m_FilePath);

		std::unordered_map<ShaderType, String> shaderSources;
		bool result = PreProcess(shaderString, shaderSources);

		if (!result)
			return false;

		if (!CheckShaderStages(shaderSources))
			return false;

		// Get cached file paths
		std::unordered_map<ShaderType, FilePath> cachedFilePaths;

		for (const auto& [type, _] : shaderSources)
		{
			cachedFilePaths[type] = Utils::GetCachedFilePath(m_FilePath, type);
		}

		// Check if need to recompile
		bool recompile = false;
		for (const auto& [_, path] : cachedFilePaths)
		{
			if (!FileSystem::Exists(path))
			{
				recompile = true;
				break;
			}
		}

		m_Compiled = true;

		forceCompile |= recompile;
		if (forceCompile)
		{
			FileSystem::CreateDirectory(Renderer::GetShaderCacheDirectory() / "Vulkan");

			shaderc::Compiler compiler;
			shaderc::CompileOptions options;
			options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
			options.SetOptimizationLevel(shaderc_optimization_level_performance);
			options.SetSourceLanguage(m_IsHlsl ? shaderc_source_language_hlsl : shaderc_source_language_glsl);
			options.SetGenerateDebugInfo();

			const std::unordered_map<String, String>& globalMacroses = Renderer::GetGlobalShaderMacroses();
			for (const auto& [name, value] : globalMacroses)
			{
				if (value.empty())
				{
					options.AddMacroDefinition(value);
				}
				else
				{
					options.AddMacroDefinition(name, value);
				}
			}

			for (const auto& [type, source] : shaderSources)
			{
				String filePathStr = m_FilePath.string();
				shaderc::SpvCompilationResult module = 
					compiler.CompileGlslToSpv(source, Utils::ShaderTypeToShaderCKind(type), filePathStr.c_str(), Utils::GetEntryPointName(type).data(), options);

				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					ATN_CORE_ERROR_TAG("Vulkan", "Shader '{}' compilation failed, error message:\n{}\n", m_Name, module.GetErrorMessage());
					m_Compiled = false;
					break;
				}

				m_SPIRVBinaries[type] = std::vector<uint32>(module.cbegin(), module.cend());

				// Save to cache
				auto& data = m_SPIRVBinaries.at(type);
				FileSystem::WriteFile(cachedFilePaths.at(type), (const char*)data.data(), data.size() * sizeof(uint32));
			}
		}
		else
		{
			for (const auto& [type, _] : shaderSources)
			{
				std::vector<byte> bytes = FileSystem::ReadFileBinary(cachedFilePaths.at(type));
				ATN_CORE_VERIFY(bytes.size() % 4 == 0);

				m_SPIRVBinaries[type] = std::vector<uint32>(bytes.size() / sizeof(uint32));
				memcpy(m_SPIRVBinaries[type].data(), bytes.data(), bytes.size());
			}
		}

		for (const auto& [type, src] : m_SPIRVBinaries)
			Reflect(type, src);

		if(forceCompile)
			ATN_CORE_INFO_TAG("Vulkan", "Shader '{}' compilation took {}", m_Name, compilationTimer.ElapsedTime());

		return true;
	}

	bool VulkanShader::CheckShaderStages(const std::unordered_map<ShaderType, String>& sources)
	{
		std::string_view errorMsg = "";

		if (sources.size() == 0)
		{
			errorMsg = "Found 0 shader stages.";
		}

		std::unordered_map<ShaderType, bool> shaderTypeExistMap;
		shaderTypeExistMap[ShaderType::VERTEX_SHADER] = false;
		shaderTypeExistMap[ShaderType::FRAGMENT_SHADER] = false;
		shaderTypeExistMap[ShaderType::GEOMETRY_SHADER] = false;
		shaderTypeExistMap[ShaderType::COMPUTE_SHADER] = false;

		for (const auto& [type, _] : sources)
		{
			shaderTypeExistMap.at(type) = true;
		}

		if (shaderTypeExistMap.at(ShaderType::VERTEX_SHADER) && !shaderTypeExistMap.at(ShaderType::FRAGMENT_SHADER))
		{
			errorMsg = "Has VERTEX_SHADER stage but no FRAGMENT_SHADER stage.";
		}

		if (!shaderTypeExistMap.at(ShaderType::VERTEX_SHADER) && shaderTypeExistMap.at(ShaderType::FRAGMENT_SHADER))
		{
			errorMsg = "Has FRAGMENT_SHADER stage but no VERTEX_SHADER stage.";
		}

		if (shaderTypeExistMap.at(ShaderType::GEOMETRY_SHADER) && (!shaderTypeExistMap.at(ShaderType::VERTEX_SHADER) || shaderTypeExistMap.at(ShaderType::FRAGMENT_SHADER)))
		{
			errorMsg = "Has GEOMETRY_SHADER stage but no VERTEX_SHADER or FRAGMENT_SHADER stage.";
		}

		if (shaderTypeExistMap.at(ShaderType::COMPUTE_SHADER) && sources.size() != 1)
		{
			errorMsg = "Compute shader must have only 1 stage - COMPUTE_SHADER.";
		}

		if (errorMsg.size() != 0)
		{
			ATN_CORE_ERROR_TAG("Vulkan", "Shader '{}' compilation failed, error message:\n{}\n", m_Name, errorMsg);
			return false;
		}

		return true;
	}

	void VulkanShader::Reflect(ShaderType type, const std::vector<uint32>& src)
	{
		spirv_cross::Compiler compiler(src);
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

		ATN_CORE_TRACE("Shader Reflect - {} {}", Utils::ShaderTypeToString(type), m_Name);
		ATN_CORE_TRACE("    {} uniform buffers", resources.uniform_buffers.size());
		ATN_CORE_TRACE("    {} resources", resources.sampled_images.size());

		ATN_CORE_TRACE("Uniform buffers:");
		for (const auto& resource : resources.uniform_buffers)
		{
			const auto& bufferType = compiler.get_type(resource.base_type_id);
			uint32 bufferSize = compiler.get_declared_struct_size(bufferType);
			uint32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

			int memberCount = bufferType.member_types.size();

			ATN_CORE_TRACE("  {}", resource.name);
			ATN_CORE_TRACE("\tBuffer size = {}", bufferSize);
			ATN_CORE_TRACE("\tBinding = {}", binding);
			ATN_CORE_TRACE("\tSet = {}", set);
			ATN_CORE_TRACE("\tMembers = {}", memberCount);

			for (int i = 0; i < memberCount; ++i)
			{
				String name = compiler.get_member_name(resource.base_type_id, i);
				size_t size = compiler.get_declared_struct_member_size(compiler.get_type(resource.base_type_id), i);
				ATN_CORE_TRACE("\t  Name = {}", name);
				ATN_CORE_TRACE("\t  Size = {}", size);
			}
		}
	}

	void VulkanShader::CreateVulkanShaderModulesAndStages()
	{
		for (const auto& [type, src] : m_SPIRVBinaries)
		{
			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = src.size() * sizeof(uint32);
			createInfo.pCode = src.data();

			VK_CHECK(vkCreateShaderModule(VulkanContext::GetDevice()->GetLogicalDevice(), &createInfo, VulkanContext::GetAllocator(), &m_VulkanShaderModules[type]));

			VkPipelineShaderStageCreateInfo shaderStageCI = {};
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.stage = VulkanUtils::GetShaderStage(type);
			shaderStageCI.module = m_VulkanShaderModules[type];
			shaderStageCI.pName = m_IsHlsl ? Utils::GetEntryPointName(type).data() : "main";

			m_PipelineShaderStages.push_back(shaderStageCI);
		}
	}

	void VulkanShader::CleanUp()
	{
		Renderer::SubmitResourceFree([shaderModules = m_VulkanShaderModules]()
			{
				for (const auto& [type, src] : shaderModules)
				{
					vkDestroyShaderModule(VulkanContext::GetDevice()->GetLogicalDevice(), shaderModules.at(type), VulkanContext::GetAllocator());
				}
			});
	}
}
