#include "VulkanComputePass.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"


namespace Athena
{
	VulkanComputePass::VulkanComputePass(const ComputePassCreateInfo& info)
	{
		m_Info = info;
	}

	VulkanComputePass::~VulkanComputePass()
	{

	}

	void VulkanComputePass::Begin(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		if (m_Info.DebugColor != LinearColor(0.f))
			Renderer::BeginDebugRegion(commandBuffer, m_Info.Name, m_Info.DebugColor);

		Renderer::Submit([instance = Ref(this), commandBuffer]()
		{
			VkCommandBuffer cmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

			for (const auto& image : instance->m_Outputs)
			{
				bool isAttachment = image->GetInfo().Usage & ImageUsage::ATTACHMENT;

				VkImageLayout newLayout = VK_IMAGE_LAYOUT_GENERAL;
				VkPipelineStageFlags sourceStage = isAttachment ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

				image.As<VulkanImage>()->RT_TransitionLayout(cmdBuffer, newLayout, sourceStage, destinationStage);
			}
		});
	}

	void VulkanComputePass::End(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		Renderer::Submit([instance = Ref(this), commandBuffer]()
		{
			VkCommandBuffer cmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

			for (const auto& image : instance->m_Outputs)
			{
				bool isAttachment = image->GetInfo().Usage & ImageUsage::ATTACHMENT;

				VkImageLayout newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
				VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

				image.As<VulkanImage>()->RT_TransitionLayout(cmdBuffer, newLayout, sourceStage, destinationStage);
			}
		});

		if (m_Info.DebugColor != LinearColor(0.f))
			Renderer::EndDebugRegion(commandBuffer);
	}
}
