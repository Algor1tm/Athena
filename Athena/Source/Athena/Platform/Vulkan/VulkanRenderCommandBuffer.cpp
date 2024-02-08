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
		case RenderCommandBufferUsage::IMMEDIATE: count = 1;
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
		Renderer::Submit([this]()
		{
			VkCommandBuffer vkCommandBuffer = GetVulkanCommandBuffer();

			if (m_Info.Usage != RenderCommandBufferUsage::IMMEDIATE)
				VK_CHECK(vkResetCommandBuffer(vkCommandBuffer, 0));

			VkCommandBufferBeginInfo cmdBufBeginInfo = {};
			cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CHECK(vkBeginCommandBuffer(vkCommandBuffer, &cmdBufBeginInfo));
		});
	}

	void VulkanRenderCommandBuffer::End()
	{
		Renderer::Submit([this]()
		{
			VK_CHECK(vkEndCommandBuffer(GetVulkanCommandBuffer()));
		});
	}

	void VulkanRenderCommandBuffer::Submit()
	{
		switch (m_Info.Usage)
		{
		case RenderCommandBufferUsage::PRESENT: SubmitForPresent(); break;
		case RenderCommandBufferUsage::IMMEDIATE: SubmitImmediate(); break;
		}
	}

	void VulkanRenderCommandBuffer::SubmitForPresent()
	{
		Renderer::Submit([instance = Ref(this)]()
		{
			ATN_PROFILE_SCOPE("VulkanRenderCommandBuffer::Submit")

			const FrameSyncData& frameData = VulkanContext::GetFrameSyncData(Renderer::GetCurrentFrameIndex());
			VkCommandBuffer commandBuffer = instance->GetVulkanCommandBuffer();

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

			{
				ATN_PROFILE_SCOPE("vkQueueSubmit")
				Timer timer = Timer();

				VK_CHECK(vkQueueSubmit(VulkanContext::GetDevice()->GetQueue(), 1, &submitInfo, frameData.RenderCompleteFence));
				Application::Get().GetStats().Renderer_QueueSubmit = timer.ElapsedTime();
			}
		});
	}

	void VulkanRenderCommandBuffer::SubmitImmediate()
	{
		Renderer::Submit([instance = Ref(this)]()
		{
			VkCommandBuffer commandBuffer = instance->GetVulkanCommandBuffer();

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = 0;

			VkFence fence;
			VK_CHECK(vkCreateFence(VulkanContext::GetLogicalDevice(), &fenceInfo, nullptr, &fence));

			VK_CHECK(vkQueueSubmit(VulkanContext::GetDevice()->GetQueue(), 1, &submitInfo, fence));

			VK_CHECK(vkWaitForFences(VulkanContext::GetLogicalDevice(), 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
			vkDestroyFence(VulkanContext::GetLogicalDevice(), fence, nullptr);
		});
	}

	VkCommandBuffer VulkanRenderCommandBuffer::GetVulkanCommandBuffer()
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
