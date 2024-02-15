#include "VulkanComputePipeline.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"


namespace Athena
{
	VulkanComputePipeline::VulkanComputePipeline(const ComputePipelineCreateInfo& info)
	{
		ATN_CORE_ASSERT(info.Shader->IsCompute(), "Compute pipeline require compute shader!");

		m_Info = info;

		DescriptorSetManagerCreateInfo setManagerInfo;
		setManagerInfo.Name = info.Name;
		setManagerInfo.Shader = m_Info.Shader;
		setManagerInfo.FirstSet = 1;
		setManagerInfo.LastSet = 4;
		m_DescriptorSetManager = Ref<DescriptorSetManager>::Create(setManagerInfo);

		auto vkShader = m_Info.Shader.As<VulkanShader>();

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage = vkShader->GetPipelineStages()[0];
		pipelineInfo.layout = vkShader->GetPipelineLayout();

		VK_CHECK(vkCreateComputePipelines(VulkanContext::GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VulkanPipeline));
		Vulkan::SetObjectDebugName(m_VulkanPipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, m_Info.Name);
	}

	VulkanComputePipeline::~VulkanComputePipeline()
	{
		Renderer::SubmitResourceFree([pipeline = m_VulkanPipeline]()
		{
			vkDestroyPipeline(VulkanContext::GetLogicalDevice(), pipeline, nullptr);
		});
	}

	void VulkanComputePipeline::Bind(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		Renderer::Submit([this, commandBuffer]()
		{
			VkCommandBuffer vkcmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();
			vkCmdBindPipeline(vkcmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_VulkanPipeline);

			m_DescriptorSetManager->RT_InvalidateAndUpdate();
			m_DescriptorSetManager->RT_BindDescriptorSets(vkcmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE);
		});
	}

	void VulkanComputePipeline::SetInput(const String& name, const Ref<RenderResource>& resource)
	{
		m_DescriptorSetManager->Set(name, resource);
	}

	void VulkanComputePipeline::Bake()
	{
		m_DescriptorSetManager->Bake();
	}
}
