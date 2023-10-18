#include "VulkanShader.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Time.h"

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
		static FilePath GetCachedFilePath(const FilePath& path, ShaderStage stage)
		{
			FilePath relativeToShaderPack = std::filesystem::relative(path, Renderer::GetShaderPackDirectory());
			FilePath cachedPath = Renderer::GetShaderCacheDirectory() / relativeToShaderPack;

			switch (stage)
			{
			case ShaderStage::VERTEX_STAGE:    cachedPath += L".cached.vert"; break;
			case ShaderStage::FRAGMENT_STAGE:  cachedPath += L".cached.frag"; break;
			case ShaderStage::GEOMETRY_STAGE:  cachedPath += L".cached.geom"; break;
			case ShaderStage::COMPUTE_STAGE:   cachedPath += L".cached.compute"; break;
			default: ATN_CORE_ASSERT(false); return "";
			}

			return cachedPath;
		}

		static shaderc_shader_kind ShaderStageToShaderCKind(ShaderStage stage)
		{
			switch (stage)
			{
			case ShaderStage::VERTEX_STAGE:    return shaderc_glsl_vertex_shader;
			case ShaderStage::FRAGMENT_STAGE:  return shaderc_glsl_fragment_shader;
			case ShaderStage::GEOMETRY_STAGE:  return shaderc_glsl_geometry_shader;
			case ShaderStage::COMPUTE_STAGE:   return shaderc_glsl_compute_shader;
			}

			ATN_CORE_ASSERT(false);
			return (shaderc_shader_kind)0;
		}


		static std::string_view ShaderStageToString(ShaderStage stage)
		{
			switch (stage)
			{
			case ShaderStage::VERTEX_STAGE: return "Vertex Stage";
			case ShaderStage::FRAGMENT_STAGE: return "Fragment Stage";
			case ShaderStage::GEOMETRY_STAGE: return "Geometry Stage";
			case ShaderStage::COMPUTE_STAGE: return "Compute Stage";
			}

			ATN_CORE_ASSERT(false);
			return "";
		}

		static bool ShaderStageFromString(const String& strType, ShaderStage& stage)
		{
			if (strType == "VERTEX_STAGE")
				stage = ShaderStage::VERTEX_STAGE;
			else if (strType == "FRAGMENT_STAGE" || strType == "PIXEL_STAGE")
				stage = ShaderStage::FRAGMENT_STAGE;
			else if (strType == "GEOMETRY_STAGE")
				stage = ShaderStage::GEOMETRY_STAGE;
			else if (strType == "COMPUTE_STAGE")
				stage = ShaderStage::COMPUTE_STAGE;
			else
				return false;

			return true;
		}

		static std::string_view GetEntryPointName(ShaderStage stage, bool isHlsl)
		{
			if (!isHlsl)
				return "main";

			switch (stage)
			{
			case ShaderStage::VERTEX_STAGE: return "VSMain";
			case ShaderStage::FRAGMENT_STAGE: return "FSMain";
			case ShaderStage::GEOMETRY_STAGE: return "GSMain";
			case ShaderStage::COMPUTE_STAGE: return "CSMain";
			}

			ATN_CORE_ASSERT(false);
			return "";
		}
	}

	class FileSystemIncluder : public shaderc::CompileOptions::IncluderInterface
	{
	public:
		virtual shaderc_include_result* GetInclude(
			const char* requested_source,   // relative src path
			shaderc_include_type type,		
			const char* requesting_source,	// dst path
			size_t include_depth) override
		{
			FilePath dstParentPath = FilePath(requesting_source).parent_path();
			FilePath srcPath = dstParentPath / requested_source;

			m_SourcePath = srcPath.string();
			m_Source = FileSystem::ReadFile(srcPath);

			shaderc_include_result* result = new shaderc_include_result();

			result->source_name = m_SourcePath.c_str();
			result->source_name_length = m_SourcePath.size();
			result->content = m_Source.c_str();
			result->content_length = m_Source.size();

			return result;
		}

		virtual void ReleaseInclude(shaderc_include_result* data) override
		{
			delete data;
		}

	private:
		String m_SourcePath;
		String m_Source;
	};



	VulkanShader::VulkanShader(const FilePath& path, const String& name)
	{
		m_FilePath = path;
		m_Name = name;

		m_IsHlsl = m_FilePath.extension() == L".hlsl";

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

	bool VulkanShader::PreProcess(const String& source, std::unordered_map<ShaderStage, String>& result)
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

			ShaderStage type;
			bool convertResult = Utils::ShaderStageFromString(typeString, type);
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
		if (ext != L".hlsl"&& ext != L".glsl")
		{
			ATN_CORE_ERROR_TAG("Vulkan", "Invalid shader extension '{}'!", m_FilePath);
			return false;
		}

		String shaderString = FileSystem::ReadFile(m_FilePath);

		std::unordered_map<ShaderStage, String> shaderSources;
		bool result = PreProcess(shaderString, shaderSources);

		if (!result)
			return false;

		if (!CheckShaderStages(shaderSources))
			return false;

		// Get cached file paths
		std::unordered_map<ShaderStage, FilePath> cachedFilePaths;

		for (const auto& [stage, _] : shaderSources)
		{
			cachedFilePaths[stage] = Utils::GetCachedFilePath(m_FilePath, stage);
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
			shaderc::Compiler compiler;
			shaderc::CompileOptions options;
			options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
			options.SetOptimizationLevel(shaderc_optimization_level_performance);
			options.SetSourceLanguage(m_IsHlsl ? shaderc_source_language_hlsl : shaderc_source_language_glsl);
			options.SetInvertY(true);
			options.SetGenerateDebugInfo();
			options.SetIncluder(std::make_unique<FileSystemIncluder>());

			const std::unordered_map<String, String>& globalMacroses = Renderer::GetGlobalShaderMacroses();
			for (const auto& [name, value] : globalMacroses)
			{
				options.AddMacroDefinition(name, value);
			}

			for (const auto& [stage, source] : shaderSources)
			{
				String filePathStr = m_FilePath.string();

				shaderc::PreprocessedSourceCompilationResult preResult =
					compiler.PreprocessGlsl(source, Utils::ShaderStageToShaderCKind(stage), filePathStr.c_str(), options);

				if (preResult.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					ATN_CORE_ERROR_TAG("Vulkan", "Shader '{}' preprocess failed, error message:\n{}\n", m_Name, preResult.GetErrorMessage());
					m_Compiled = false;
					break;
				}

				String preProcessedSource = String(preResult.begin(), preResult.end());

				shaderc::SpvCompilationResult module = 
					compiler.CompileGlslToSpv(preProcessedSource, Utils::ShaderStageToShaderCKind(stage), filePathStr.c_str(), Utils::GetEntryPointName(stage, m_IsHlsl).data(), options);

				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					ATN_CORE_ERROR_TAG("Vulkan", "Shader '{}' compilation failed, error message:\n{}\n", m_Name, module.GetErrorMessage());
					m_Compiled = false;
					break;
				}

				m_SPIRVBinaries[stage] = std::vector<uint32>(module.cbegin(), module.cend());

				// Save to cache
				auto& data = m_SPIRVBinaries.at(stage);
				FileSystem::WriteFile(cachedFilePaths.at(stage), (const char*)data.data(), data.size() * sizeof(uint32));
			}
		}
		else
		{
			for (const auto& [stage, _] : shaderSources)
			{
				std::vector<byte> bytes = FileSystem::ReadFileBinary(cachedFilePaths.at(stage));

				m_SPIRVBinaries[stage] = std::vector<uint32>(bytes.size() / sizeof(uint32));
				memcpy(m_SPIRVBinaries[stage].data(), bytes.data(), bytes.size());
			}
		}

		for (const auto& [stage, src] : m_SPIRVBinaries)
			Reflect(stage, src);

		if(forceCompile)
			ATN_CORE_INFO_TAG("Vulkan", "Shader '{}' compilation took {}", m_Name, compilationTimer.ElapsedTime());

		return true;
	}

	bool VulkanShader::CheckShaderStages(const std::unordered_map<ShaderStage, String>& sources)
	{
		std::string_view errorMsg = "";

		if (sources.size() == 0)
		{
			errorMsg = "Found 0 shader stages.";
		}

		std::unordered_map<ShaderStage, bool> ShaderStageExistMap;
		ShaderStageExistMap[ShaderStage::VERTEX_STAGE] = false;
		ShaderStageExistMap[ShaderStage::FRAGMENT_STAGE] = false;
		ShaderStageExistMap[ShaderStage::GEOMETRY_STAGE] = false;
		ShaderStageExistMap[ShaderStage::COMPUTE_STAGE] = false;

		for (const auto& [stage, _] : sources)
		{
			ShaderStageExistMap.at(stage) = true;
		}

		if (ShaderStageExistMap.at(ShaderStage::VERTEX_STAGE) && !ShaderStageExistMap.at(ShaderStage::FRAGMENT_STAGE))
		{
			errorMsg = "Has VERTEX_STAGE but no FRAGMENT_STAGE.";
		}

		if (!ShaderStageExistMap.at(ShaderStage::VERTEX_STAGE) && ShaderStageExistMap.at(ShaderStage::FRAGMENT_STAGE))
		{
			errorMsg = "Has FRAGMENT_STAGE but no VERTEX_STAGE.";
		}

		if (ShaderStageExistMap.at(ShaderStage::GEOMETRY_STAGE) && (!ShaderStageExistMap.at(ShaderStage::VERTEX_STAGE) || ShaderStageExistMap.at(ShaderStage::FRAGMENT_STAGE)))
		{
			errorMsg = "Has GEOMETRY_STAGE but no VERTEX_SHADER or FRAGMENT_STAGE.";
		}

		if (ShaderStageExistMap.at(ShaderStage::COMPUTE_STAGE) && sources.size() != 1)
		{
			errorMsg = "Compute shader must have only 1 stage - COMPUTE_STAGE.";
		}

		if (errorMsg.size() != 0)
		{
			ATN_CORE_ERROR_TAG("Vulkan", "Shader '{}' compilation failed, error message:\n{}\n", m_Name, errorMsg);
			return false;
		}

		return true;
	}

	void VulkanShader::Reflect(ShaderStage type, const std::vector<uint32>& src)
	{
		spirv_cross::Compiler compiler(src);
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

		ATN_CORE_TRACE("Shader Reflect - {} {}", Utils::ShaderStageToString(type), m_Name);
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
		for (const auto& [stage, src] : m_SPIRVBinaries)
		{
			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = src.size() * sizeof(uint32);
			createInfo.pCode = src.data();

			VK_CHECK(vkCreateShaderModule(VulkanContext::GetLogicalDevice(), &createInfo, VulkanContext::GetAllocator(), &m_VulkanShaderModules[stage]));

			VkPipelineShaderStageCreateInfo shaderStageCI = {};
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.stage = Utils::GetVulkanShaderStage(stage);
			shaderStageCI.module = m_VulkanShaderModules.at(stage);
			shaderStageCI.pName = Utils::GetEntryPointName(stage, m_IsHlsl).data();

			m_PipelineShaderStages.push_back(shaderStageCI);
		}
	}

	void VulkanShader::CleanUp()
	{
		Renderer::SubmitResourceFree([shaderModules = m_VulkanShaderModules]()
			{
				for (const auto& [stage, src] : shaderModules)
				{
					vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), shaderModules.at(stage), VulkanContext::GetAllocator());
				}
			});
	}
}
