#include "VulkanShader.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Time.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"



namespace Athena
{
	VulkanShader::VulkanShader(const FilePath& path, const String& name)
	{
		m_FilePath = path;
		m_Name = name;

		ShaderCompiler compiler(m_FilePath, m_Name);
		m_Compiled = compiler.CompileOrGetFromCache(false);

		if (m_Compiled)
			CreateVulkanShaderModulesAndStages(compiler);
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

		ShaderCompiler compiler(m_FilePath, m_Name);
		m_Compiled = compiler.CompileOrGetFromCache(true);

		if (m_Compiled)
			CreateVulkanShaderModulesAndStages(compiler);
	}

	void VulkanShader::CreateVulkanShaderModulesAndStages(const ShaderCompiler& compiler)
	{
		const ShaderBinaries& binaries = compiler.GetBinaries();

		for (const auto& [stage, src] : binaries)
		{
			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = src.size() * sizeof(uint32);
			createInfo.pCode = src.data();

			VK_CHECK(vkCreateShaderModule(VulkanContext::GetLogicalDevice(), &createInfo, VulkanContext::GetAllocator(), &m_VulkanShaderModules[stage]));

			VkPipelineShaderStageCreateInfo shaderStageCI = {};
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.stage = VulkanUtils::GetShaderStage(stage);
			shaderStageCI.module = m_VulkanShaderModules.at(stage);
			shaderStageCI.pName = compiler.GetEntryPoint(stage).data();

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
