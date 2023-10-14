#include "VulkanShader.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/VulkanRenderer.h"
#include "Athena/Platform/Vulkan/VulkanDebug.h"

#if defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif


namespace Athena
{
	static FilePath GetCachedFileName(const FilePath& path, ShaderType type)
	{
		FilePath result = path;
		switch (type)
		{
		case ShaderType::VERTEX_SHADER:    return result.replace_extension("glsl.cached.vert");
		case ShaderType::FRAGMENT_SHADER:  return result.replace_extension("glsl.cached.frag");
		case ShaderType::GEOMETRY_SHADER:  return result.replace_extension("glsl.cached.geom");
		case ShaderType::COMPUTE_SHADER:   return result.replace_extension("glsl.cached.compute");
		}

		ATN_CORE_ASSERT(false);
		return "";
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

	static VkShaderStageFlagBits ShaderTypeToVkShaderStage(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::VERTEX_SHADER: return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderType::FRAGMENT_SHADER: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderType::GEOMETRY_SHADER: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderType::COMPUTE_SHADER: return VK_SHADER_STAGE_COMPUTE_BIT;
		}

		ATN_CORE_ASSERT(false);
		return (VkShaderStageFlagBits)0;
	}


	VulkanShader::VulkanShader(const FilePath& path, const String& name)
	{
		m_FilePath = path;
		m_Name = name;

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

	bool VulkanShader::CompileOrGetBinaries(bool forceCompile)
	{
		if (!FileSystem::Exists(m_FilePath))
		{
			ATN_CORE_ERROR_TAG("Vulkan", "Invalid filepath for shader {}!", m_FilePath);
			return false;
		}

		String result = FileSystem::ReadFile(m_FilePath);
		std::unordered_map<ShaderType, String> shaderSources = PreProcess(result);

		m_Compiled = false;
		if (!CheckShaderStages(shaderSources))
			return false;

		// Get cached file paths
		std::unordered_map<ShaderType, FilePath> cachedFilePaths;

		for (const auto& [type, _] : shaderSources)
		{
			cachedFilePaths[type] = Renderer::GetShaderCacheDirectory() / GetCachedFileName(m_FilePath.filename(), type);
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

		if (forceCompile || recompile)
		{
			shaderc::Compiler compiler;
			shaderc::CompileOptions options;
			options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
			options.SetOptimizationLevel(shaderc_optimization_level_performance);
			//options.SetSourceLanguage(shaderc_source_language_hlsl);
			options.SetGenerateDebugInfo();

			String filePathStr = m_FilePath.string();
			for (const auto& [type, source] : shaderSources)
			{
				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, ShaderTypeToShaderCKind(type), filePathStr.c_str(), options);

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
				ATN_CORE_ASSERT(bytes.size() % 4 == 0);

				m_SPIRVBinaries[type] = std::vector<uint32>(bytes.size() / sizeof(uint32));
				memcpy(m_SPIRVBinaries[type].data(), bytes.data(), bytes.size());
			}
		}

		for (const auto& [type, src] : m_SPIRVBinaries)
			Reflect(type, src);

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

		ATN_CORE_TRACE("Shader Reflect - {} {}", ShaderTypeToString(type), m_Name);
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
			shaderStageCI.stage = ShaderTypeToVkShaderStage(type);
			shaderStageCI.module = m_VulkanShaderModules[type];
			shaderStageCI.pName = "main";

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
