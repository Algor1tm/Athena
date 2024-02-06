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
		Renderer::Submit([instance = Ref(this), commandBuffer]()
		{
			for (const auto& image : instance->m_Info.Outputs)
			{
				bool isAttachment = image->GetInfo().Usage & ImageUsage::ATTACHMENT;

				VkImageLayout oldLayout = isAttachment ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
				VkImageLayout newLayout = VK_IMAGE_LAYOUT_GENERAL;

				VkPipelineStageFlags sourceStage = isAttachment ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.oldLayout = oldLayout;
				barrier.newLayout = newLayout;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = image.As<VulkanImage>()->GetVulkanImage();
				barrier.subresourceRange.aspectMask = Vulkan::GetImageAspectMask(image->GetInfo().Format);
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = image->GetInfo().MipLevels;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = image->GetInfo().Layers;

				vkCmdPipelineBarrier(
					commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(),
					sourceStage, destinationStage,
					VK_DEPENDENCY_BY_REGION_BIT,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);
			}
		});
	}

	void VulkanComputePass::End(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		Renderer::Submit([instance = Ref(this), commandBuffer]()
		{
			for (const auto& image : instance->m_Info.Outputs)
			{
				bool isAttachment = image->GetInfo().Usage & ImageUsage::ATTACHMENT;

				VkImageLayout oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				VkImageLayout newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
				VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.oldLayout = oldLayout;
				barrier.newLayout = newLayout;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = image.As<VulkanImage>()->GetVulkanImage();
				barrier.subresourceRange.aspectMask = Vulkan::GetImageAspectMask(image->GetInfo().Format);
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = image->GetInfo().MipLevels;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = image->GetInfo().Layers;

				vkCmdPipelineBarrier(
					commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(),
					sourceStage, destinationStage,
					VK_DEPENDENCY_BY_REGION_BIT,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);
			}
		});
	}
}
