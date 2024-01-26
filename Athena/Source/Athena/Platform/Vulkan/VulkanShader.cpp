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

			std::vector<VkDescriptorSetLayoutBinding> bindings;

			for (const auto& [name, ubo] : m_ReflectionData.UniformBuffers)
			{
				VkDescriptorSetLayoutBinding uboLayoutBinding = {};
				uboLayoutBinding.binding = ubo.Binding;
				uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				uboLayoutBinding.descriptorCount = 1;
				uboLayoutBinding.stageFlags = VulkanUtils::GetShaderStageFlags(ubo.StageFlags);
				uboLayoutBinding.pImmutableSamplers = nullptr;

				bindings.push_back(uboLayoutBinding);
			}

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = bindings.size();
			layoutInfo.pBindings = bindings.data();

			VK_CHECK(vkCreateDescriptorSetLayout(VulkanContext::GetLogicalDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout));

			VkDescriptorSetLayout setLayout = m_DescriptorSetLayout;
			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &setLayout;

			VkPushConstantRange range = {};
			const auto& pushConstant = m_ReflectionData.PushConstant;

			if (pushConstant.Enabled)
			{
				range.offset = 0;
				range.size = pushConstant.Size;
				range.stageFlags = VulkanUtils::GetShaderStageFlags(pushConstant.StageFlags);

				pipelineLayoutInfo.pushConstantRangeCount = 1;
				pipelineLayoutInfo.pPushConstantRanges = &range;
			}
			else
			{
				pipelineLayoutInfo.pushConstantRangeCount = 0;
				pipelineLayoutInfo.pPushConstantRanges = nullptr;
			}

			VK_CHECK(vkCreatePipelineLayout(VulkanContext::GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));
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
		Renderer::SubmitResourceFree([shaderModules = m_VulkanShaderModules, setLayout = m_DescriptorSetLayout, pipelineLayout = m_PipelineLayout]()
		{
			for (const auto& [stage, src] : shaderModules)
			{
				vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), shaderModules.at(stage), nullptr);
			}

			vkDestroyDescriptorSetLayout(VulkanContext::GetLogicalDevice(), setLayout, nullptr);
			vkDestroyPipelineLayout(VulkanContext::GetLogicalDevice(), pipelineLayout, nullptr);
		});
	}
}
