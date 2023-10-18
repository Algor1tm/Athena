#include "VulkanCommandBuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanCommandBuffer::VulkanCommandBuffer(CommandBufferUsage usage)
		: m_Usage(usage)
	{
		uint32 count = 0;
		switch (m_Usage)
		{
		case CommandBufferUsage::PRESENT: count = Renderer::GetFramesInFlight(); break;
		case CommandBufferUsage::IMMEDIATE: count = 1;
		}

		VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
		cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocInfo.commandPool = VulkanContext::GetCommandPool();
		cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocInfo.commandBufferCount = count;

		m_VkCommandBuffer.resize(count);
		VK_CHECK(vkAllocateCommandBuffers(VulkanContext::GetLogicalDevice(), &cmdBufAllocInfo, m_VkCommandBuffer.data()));
	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		Renderer::SubmitResourceFree([commandBuffers = m_VkCommandBuffer]()
			{
				vkFreeCommandBuffers(VulkanContext::GetLogicalDevice(), VulkanContext::GetCommandPool(), commandBuffers.size(), commandBuffers.data());
			});
	}

	void VulkanCommandBuffer::Begin()
	{
		VkCommandBuffer vkCommandBuffer = GetVulkanCommandBuffer();

		VK_CHECK(vkResetCommandBuffer(vkCommandBuffer, 0));

		VkCommandBufferBeginInfo cmdBufBeginInfo = {};
		cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK(vkBeginCommandBuffer(vkCommandBuffer, &cmdBufBeginInfo));

		VulkanContext::SetActiveCommandBuffer(vkCommandBuffer);
	}

	void VulkanCommandBuffer::End()
	{
		VK_CHECK(vkEndCommandBuffer(GetVulkanCommandBuffer()));

		VulkanContext::SetActiveCommandBuffer(VK_NULL_HANDLE);
	}

	void VulkanCommandBuffer::Flush()
	{
		switch (m_Usage)
		{
		case CommandBufferUsage::PRESENT: FlushForPresent(); break;
		case CommandBufferUsage::IMMEDIATE: FlushImmediate(); break;
		}
	}

	void VulkanCommandBuffer::FlushForPresent()
	{
		const FrameSyncData& frameData = VulkanContext::GetFrameSyncData(Renderer::GetCurrentFrameIndex());
		VkCommandBuffer commandBuffer = GetVulkanCommandBuffer();

		VkPipelineStageFlags waitStage[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		// Submit commands to queue
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &frameData.ImageAcquiredSemaphore;
		submitInfo.pWaitDstStageMask = waitStage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &frameData.RenderCompleteSemaphore;

		VK_CHECK(vkQueueSubmit(VulkanContext::GetDevice()->GetQueue(), 1, &submitInfo, frameData.RenderCompleteFence));
	}

	void VulkanCommandBuffer::FlushImmediate()
	{
		VkCommandBuffer commandBuffer = GetVulkanCommandBuffer();

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = 0;

		VkFence fence;
		VK_CHECK(vkCreateFence(VulkanContext::GetLogicalDevice(), &fenceInfo, VulkanContext::GetAllocator(), &fence));

		VK_CHECK(vkQueueSubmit(VulkanContext::GetDevice()->GetQueue(), 1, &submitInfo, fence));

		VK_CHECK(vkWaitForFences(VulkanContext::GetLogicalDevice(), 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

		vkDestroyFence(VulkanContext::GetLogicalDevice(), fence, VulkanContext::GetAllocator());
	}

	VkCommandBuffer VulkanCommandBuffer::GetVulkanCommandBuffer()
	{
		switch (m_Usage)
		{
		case CommandBufferUsage::PRESENT: return m_VkCommandBuffer[Renderer::GetCurrentFrameIndex()];
		case CommandBufferUsage::IMMEDIATE: return m_VkCommandBuffer[0];
		}

		return VK_NULL_HANDLE;
	}
}
