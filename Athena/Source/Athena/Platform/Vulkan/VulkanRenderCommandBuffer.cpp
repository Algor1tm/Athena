#include "VulkanRenderCommandBuffer.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Core/Application.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanRenderCommandBuffer::VulkanRenderCommandBuffer(const RenderCommandBufferCreateInfo& info)
	{
		m_Info = info;

		uint32 count = 0;
		switch (m_Info.Usage)
		{
		case RenderCommandBufferUsage::PRESENT: count = Renderer::GetFramesInFlight(); break;
		case RenderCommandBufferUsage::IMMEDIATE: count = 1; break;
		}

		VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
		cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocInfo.commandPool = VulkanContext::GetCommandPool();
		cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocInfo.commandBufferCount = count;

		m_CommandBuffers.resize(count);
		VK_CHECK(vkAllocateCommandBuffers(VulkanContext::GetLogicalDevice(), &cmdBufAllocInfo, m_CommandBuffers.data()));

		for (uint32 i = 0; i < count; ++i)
			Vulkan::SetObjectDebugName(m_CommandBuffers[i], VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, std::format("{}_{}", m_Info.Name, i));
	}

	VulkanRenderCommandBuffer::~VulkanRenderCommandBuffer()
	{
		Renderer::SubmitResourceFree([commandBuffers = m_CommandBuffers]()
		{
			vkFreeCommandBuffers(VulkanContext::GetLogicalDevice(), VulkanContext::GetCommandPool(), commandBuffers.size(), commandBuffers.data());
		});
	}

	void VulkanRenderCommandBuffer::Begin()
	{
		VkCommandBuffer vkCommandBuffer = GetActiveCommandBuffer();

		if (m_Info.Usage != RenderCommandBufferUsage::IMMEDIATE)
			VK_CHECK(vkResetCommandBuffer(vkCommandBuffer, 0));

		VkCommandBufferBeginInfo cmdBufBeginInfo = {};
		cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK(vkBeginCommandBuffer(vkCommandBuffer, &cmdBufBeginInfo));
	}

	void VulkanRenderCommandBuffer::End()
	{
		VK_CHECK(vkEndCommandBuffer(GetActiveCommandBuffer()));
	}

	void VulkanRenderCommandBuffer::Submit(bool wait)
	{
		switch (m_Info.Usage)
		{
		case RenderCommandBufferUsage::PRESENT: SubmitForPresent(); break;
		case RenderCommandBufferUsage::IMMEDIATE: SubmitImmediate(wait); break;
		}
	}

	void VulkanRenderCommandBuffer::SubmitForPresent()
	{
		ATN_PROFILE_FUNC();

		const FrameSyncData& frameData = VulkanContext::GetFrameSyncData(Renderer::GetCurrentFrameIndex());
		const FrameSyncData& frameImageData = VulkanContext::GetFrameSyncData(Application::Get().GetWindow().GetSwapChain()->GetCurrentImageIndex());
		VkCommandBuffer commandBuffer = GetActiveCommandBuffer();

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
		submitInfo.pSignalSemaphores = &frameImageData.RenderCompleteSemaphore;

		{
			ATN_PROFILE_SCOPE("vkQueueSubmit");
			Timer timer = Timer();

			VulkanContext::GetDevice()->QueueSubmit(&submitInfo, frameData.RenderCompleteFence);
			Application::Get().GetStats().Renderer_QueueSubmit = timer.ElapsedTime();
		}
	}

	void VulkanRenderCommandBuffer::SubmitImmediate(bool wait)
	{
		VkCommandBuffer commandBuffer = GetActiveCommandBuffer();

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		if (wait)
		{
			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = 0;

			VkFence fence;
			VK_CHECK(vkCreateFence(VulkanContext::GetLogicalDevice(), &fenceInfo, nullptr, &fence));

			VulkanContext::GetDevice()->QueueSubmit(&submitInfo, fence);

			VK_CHECK(vkWaitForFences(VulkanContext::GetLogicalDevice(), 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
			vkDestroyFence(VulkanContext::GetLogicalDevice(), fence, nullptr);
		}
		else
		{
			VulkanContext::GetDevice()->QueueSubmit(&submitInfo, VK_NULL_HANDLE);
		}
	}

	VkCommandBuffer VulkanRenderCommandBuffer::GetActiveCommandBuffer()
	{
		ATN_CORE_ASSERT(!m_CommandBuffers.empty());

		switch (m_Info.Usage)
		{
		case RenderCommandBufferUsage::PRESENT: return m_CommandBuffers[Renderer::GetCurrentFrameIndex()];
		case RenderCommandBufferUsage::IMMEDIATE: return m_CommandBuffers[0];
		}

		return VK_NULL_HANDLE;
	}
}
