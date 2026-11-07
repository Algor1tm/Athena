#include "VulkanCommandBuffer.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanCommandContext.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"

namespace Athena
{
	VulkanCommandBuffer::VulkanCommandBuffer(VkCommandPool pool)
	{
		VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
		cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocInfo.commandPool = pool;
		cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocInfo.commandBufferCount = 1;

		VK_CHECK(vkAllocateCommandBuffers(Vulkan::GetLogicalDevice(), &cmdBufAllocInfo, &CmdBuffer));

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = 0;

		VK_CHECK(vkCreateFence(Vulkan::GetLogicalDevice(), &fenceInfo, nullptr, &SignalFence));

		State = CommandBufferState::Idle;
	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		ensuref(State == CommandBufferState::Idle);

		if(SignalFence)
			vkDestroyFence(Vulkan::GetLogicalDevice(), SignalFence, nullptr);

		if(CmdBuffer)
			VK_CHECK(vkResetCommandBuffer(CmdBuffer, 0));
	}

	void VulkanCommandBuffer::AddDependency(VkSemaphore semaphore, uint64 value, VkPipelineStageFlags2 waitDstStage)
	{
		VkSemaphoreSubmitInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		semaphoreInfo.semaphore = semaphore;
		semaphoreInfo.value = value;
		semaphoreInfo.stageMask = waitDstStage;
		semaphoreInfo.deviceIndex = 0;

		WaitSemaphores.push_back(semaphoreInfo);
	}

	void VulkanCommandBuffer::AddSignal(VkSemaphore semaphore, uint64 value, VkPipelineStageFlags2 signalSrcStage)
	{
		VkSemaphoreSubmitInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		semaphoreInfo.semaphore = semaphore;
		semaphoreInfo.value = value;
		semaphoreInfo.stageMask = signalSrcStage;
		semaphoreInfo.deviceIndex = 0;

		SignalSemaphores.push_back(semaphoreInfo);
	}

	void VulkanCommandBuffer::WaitCompletion()
	{
		TRACY_PROFILE_FUNC();

		if (State == CommandBufferState::Submitted)
		{
			VK_CHECK(vkWaitForFences(Vulkan::GetLogicalDevice(), 1, &SignalFence, VK_TRUE, VULKAN_DEFAULT_FENCE_TIMEOUT));
			OnCompletion();
		}
	}

	void VulkanCommandBuffer::OnCompletion()
	{
		TRACY_PROFILE_FUNC();
		ensuref(State == CommandBufferState::Submitted);

		vkResetFences(Vulkan::GetLogicalDevice(), 1, &SignalFence);
		WaitSemaphores.clear();
		SignalSemaphores.clear();
		OnCompletionCallbacks.Flush();
		State = CommandBufferState::Idle;
		LastUsedTime = Time::GetSystemCurrentTime();
	}

	void VulkanCommandBuffer::Begin()
	{
		ensuref(State == CommandBufferState::Idle);

		VK_CHECK(vkResetCommandBuffer(CmdBuffer, 0));

		VkCommandBufferBeginInfo cmdBufBeginInfo = {};
		cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK(vkBeginCommandBuffer(CmdBuffer, &cmdBufBeginInfo));
		State = CommandBufferState::RecordingCommands;
	}

	void VulkanCommandBuffer::End()
	{
		ensuref(State == CommandBufferState::RecordingCommands);

		VK_CHECK(vkEndCommandBuffer(CmdBuffer));
	}

	void VulkanCommandBuffer::Submit(VkQueue queue)
	{
		TRACY_PROFILE_FUNC();
		ensuref(State == CommandBufferState::RecordingCommands);

		VkCommandBufferSubmitInfo cmdBufferInfo = {};
		cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		cmdBufferInfo.commandBuffer = CmdBuffer;
		cmdBufferInfo.deviceMask = 0;

		VkSubmitInfo2 submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.flags = 0;
		submitInfo.waitSemaphoreInfoCount = WaitSemaphores.size();
		submitInfo.pWaitSemaphoreInfos = WaitSemaphores.data();
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &cmdBufferInfo;
		submitInfo.signalSemaphoreInfoCount = SignalSemaphores.size();
		submitInfo.pSignalSemaphoreInfos = SignalSemaphores.data();

		VK_CHECK(vkQueueSubmit2(queue, 1, &submitInfo, SignalFence));
		State = CommandBufferState::Submitted;
	}
}
