#include "VulkanShader.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanShader::VulkanShader(const FilePath& path, const String& name)
	{
		m_FilePath = path;
		m_Name = name;
		CompileOrGetFromCache(false);
	}

	VulkanShader::VulkanShader(const FilePath& path)
		: VulkanShader(path, path.stem().string())
	{

	}

	VulkanShader::~VulkanShader()
	{
		CleanUp();
	}

	void VulkanShader::Reload()
	{
		CleanUp();
		CompileOrGetFromCache(true);
	}

	void VulkanShader::CompileOrGetFromCache(bool forceCompile)
	{
		Renderer::Submit([this, forceCompile]()
		{
			ShaderCompiler compiler(m_FilePath, m_Name);
			m_IsCompiled = compiler.CompileOrGetFromCache(forceCompile);
			m_ReflectionData = compiler.Reflect();

			if (m_IsCompiled)
				CreateVulkanShaderModulesAndStages(compiler);
		});
	}

	void VulkanShader::CreateVulkanShaderModulesAndStages(const ShaderCompiler& compiler)
	{
		const ShaderBinaries& binaries = compiler.GetBinaries();

		for (const auto& [stage, src] : binaries)
		{
			VkShaderModuleCreateInfo moduleCreateInfo = {};
			moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleCreateInfo.codeSize = src.size() * sizeof(uint32);
			moduleCreateInfo.pCode = src.data();

			VK_CHECK(vkCreateShaderModule(VulkanContext::GetLogicalDevice(), &moduleCreateInfo, nullptr, &m_VulkanShaderModules[stage]));

			VkPipelineShaderStageCreateInfo shaderStageInfo = {};
			shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageInfo.stage = VulkanUtils::GetShaderStage(stage);
			shaderStageInfo.module = m_VulkanShaderModules.at(stage);
			shaderStageInfo.pName = compiler.GetEntryPoint(stage).data();

			m_PipelineShaderStages.push_back(shaderStageInfo);
		}
	}

	void VulkanShader::CleanUp()
	{
		Renderer::SubmitResourceFree([shaderModules = m_VulkanShaderModules]()
		{
			for (const auto& [stage, src] : shaderModules)
			{
				vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), shaderModules.at(stage), nullptr);
			}
		});
	}
}
