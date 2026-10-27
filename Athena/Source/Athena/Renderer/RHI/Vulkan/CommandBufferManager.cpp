#include "CommandBufferManager.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"

#define COMMAND_BUFFER_DELETE_DELAY 1 // seconds


namespace Athena
{
	VulkanCommandBuffer::VulkanCommandBuffer(VkCommandPool pool)
		: State(CommandBufferState::Idle),
		OnCompletionCallbacks(10 * 1024), // 10Kb
		LastUsedTime(0)
	{
		VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
		cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocInfo.commandPool = pool;
		cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocInfo.commandBufferCount = 1;

		VK_CHECK(vkAllocateCommandBuffers(Vulkan::GetLogicalDevice(), &cmdBufAllocInfo, &CommandBuffer));

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = 0;

		VK_CHECK(vkCreateFence(Vulkan::GetLogicalDevice(), &fenceInfo, nullptr, &SignalFence));
	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		vkDestroyFence(Vulkan::GetLogicalDevice(), SignalFence, nullptr);
		VK_CHECK(vkResetCommandBuffer(CommandBuffer, 0));
	}

	void VulkanCommandBuffer::Begin()
	{
		VK_CHECK(vkResetCommandBuffer(CommandBuffer, 0));

		VkCommandBufferBeginInfo cmdBufBeginInfo = {};
		cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &cmdBufBeginInfo));

		State = CommandBufferState::RecordingCommands;
	}

	void VulkanCommandBuffer::End()
	{
		VK_CHECK(vkEndCommandBuffer(CommandBuffer));
	}

	void VulkanCommandBuffer::OnCompletion()
	{
		TRACY_PROFILE_FUNC();

		vkResetFences(Vulkan::GetLogicalDevice(), 1, &SignalFence);

		OnCompletionCallbacks.Flush();
		State = CommandBufferState::Idle;
		LastUsedTime = Time::GetSystemCurrentTime();
	}

	void CommandBufferManager::Init()
	{
		VkCommandPoolCreateInfo commandPoolCI = {};
		commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCI.queueFamilyIndex = VulkanRHI::Get()->GetDevice()->GetQueueFamily();
		commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK(vkCreateCommandPool(Vulkan::GetLogicalDevice(), &commandPoolCI, nullptr, &m_Pool));

		VK_SET_OBJECT_NAME(m_Pool, VK_OBJECT_TYPE_COMMAND_POOL, "MainCommandPool");

		for (uint32 i = 0; i < Vulkan::GetMaxFramesInFlight(); ++i)
		{
			VulkanCommandBuffer* buffer = new VulkanCommandBuffer(m_Pool);
			m_PresentCommandBuffers.push_back(buffer);

			VkSemaphore semaphore;

			VkSemaphoreCreateInfo semaphoreCI = {};
			semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VK_CHECK(vkCreateSemaphore(Vulkan::GetLogicalDevice(), &semaphoreCI, nullptr, &semaphore));
			m_PresentCmdBufferSemaphores.push_back(semaphore);

			VK_SET_OBJECT_NAME(buffer->CommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, std::format("CmdBufferForPresent_{}", i));
			VK_SET_OBJECT_NAME(buffer->SignalFence, VK_OBJECT_TYPE_FENCE, std::format("CmdBufferForPresentFence_{}", i));
			VK_SET_OBJECT_NAME(semaphore, VK_OBJECT_TYPE_SEMAPHORE, std::format("CmdBufferForPresentSemaphore_{}", i));
		}


#if ATN_ENABLE_TRACY_GPU
		VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
		cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocInfo.commandPool = m_Pool;
		cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocInfo.commandBufferCount = 1;

		VkCommandBuffer tracyInitCmdBuffer;
		VK_CHECK(vkAllocateCommandBuffers(Vulkan::GetLogicalDevice(), &cmdBufAllocInfo, &tracyInitCmdBuffer));

		VulkanDevice* device = VulkanRHI::Get()->GetDevice();
		const DeviceExtensionsInfo& extensionsInfo = device->GetExtensionsInfo();

		if (extensionsInfo.CalibratedTimestampsEXTSupported)
		{
			m_TracyContext = TracyVkContextCalibrated(device->GetPhysicalDevice(), device->GetLogicalDevice(), device->GetQueue(), tracyInitCmdBuffer,
				extensionsInfo.vkGetPhysicalDeviceCalibrateableTimeDomainsEXT, extensionsInfo.vkGetCalibratedTimestampsEXT);

			ATN_LOG_INFO(Vulkan, "Initialized Tracy Vulkan calibrated context!");
		}
		else
		{
			m_TracyContext = TracyVkContext(device->GetPhysicalDevice(), device->GetLogicalDevice(), device->GetQueue(), tracyInitCmdBuffer);

			ATN_LOG_INFO(Vulkan, "Initialized Tracy Vulkan default context!");
		}

		VK_CHECK(vkResetCommandBuffer(tracyInitCmdBuffer, 0));
#endif
	}

	void CommandBufferManager::Destroy()
	{
#if ATN_ENABLE_TRACY_GPU
		TracyVkDestroy(m_TracyContext);
#endif

		for (uint32 i = 0; i < Vulkan::GetMaxFramesInFlight(); ++i)
		{
			vkDestroySemaphore(Vulkan::GetLogicalDevice(), m_PresentCmdBufferSemaphores[i], nullptr);

			VulkanCommandBuffer* buffer = m_PresentCommandBuffers[i];
			WaitForCmdBuffer(buffer);
			delete buffer;
		}

		for (uint32 i = 0; i < m_CommandBuffers.size(); ++i)
		{
			VulkanCommandBuffer* buffer = m_CommandBuffers[i];
			WaitForCmdBuffer(buffer);
			delete buffer;
		}

		m_CommandBuffers.clear();
		vkDestroyCommandPool(Vulkan::GetLogicalDevice(), m_Pool, nullptr);
	}

	VulkanCommandBuffer* CommandBufferManager::GetCommandBuffer()
	{
		VulkanCommandBuffer* buffer = FindFreeCommandBuffer();
		VK_SET_OBJECT_NAME(buffer->CommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "GeneralCommandBuffer");
		VK_SET_OBJECT_NAME(buffer->SignalFence, VK_OBJECT_TYPE_FENCE, "GeneralCommandBufferFence");

		buffer->Begin();

		return buffer;
	}

	VulkanCommandBuffer* CommandBufferManager::GetTransferCommandBuffer()
	{
		if (!m_TransferCommandBuffer)
		{
			m_TransferCommandBuffer = FindFreeCommandBuffer();
			VK_SET_OBJECT_NAME(m_TransferCommandBuffer->CommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "TransferCommandBuffer");
			VK_SET_OBJECT_NAME(m_TransferCommandBuffer->SignalFence, VK_OBJECT_TYPE_FENCE, "TransferCommandBufferFence");

			m_TransferCommandBuffer->Begin();
		}

		return m_TransferCommandBuffer;
	}

	VulkanCommandBuffer* CommandBufferManager::GetPresentCommandBuffer()
	{
		VulkanCommandBuffer* commandBuffer = m_PresentCommandBuffers[VulkanRHI::Get()->GetFrameResourcesIndex()];

		if (commandBuffer->State == CommandBufferState::Idle)
		{
			commandBuffer->Begin();
		}

		return commandBuffer;
	}

	VkSemaphore	CommandBufferManager::GetPresentCmdBufferSemaphore(uint32 index)
	{
		return m_PresentCmdBufferSemaphores[index];
	}

	void CommandBufferManager::WaitForCmdBuffer(VulkanCommandBuffer* commandBuffer)
	{
		TRACY_PROFILE_FUNC();

		if (commandBuffer->State == CommandBufferState::Submitted)
		{
			VK_CHECK(vkWaitForFences(Vulkan::GetLogicalDevice(), 1, &commandBuffer->SignalFence, VK_TRUE, VULKAN_DEFAULT_FENCE_TIMEOUT));
			commandBuffer->OnCompletion();
		}
	}

	void CommandBufferManager::Submit(VulkanCommandBuffer* commandBuffer)
	{
		TRACY_PROFILE_FUNC();

		ensuref(m_TransferCommandBuffer != commandBuffer && commandBuffer != nullptr);
		ensuref(commandBuffer->State == CommandBufferState::RecordingCommands);

		if (m_TransferCommandBuffer)
		{
			SubmitTransfer();
		}

		commandBuffer->End();

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer->CommandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		VK_CHECK(vkQueueSubmit(VulkanRHI::Get()->GetDevice()->GetQueue(), 1, &submitInfo, commandBuffer->SignalFence));
		commandBuffer->State = CommandBufferState::Submitted;
	}

	void CommandBufferManager::SubmitTransfer()
	{
		TRACY_PROFILE_FUNC();

		if (!m_TransferCommandBuffer)
			return;

		VulkanCommandBuffer* uploadBuffer = m_TransferCommandBuffer;
		m_TransferCommandBuffer = nullptr;

		Submit(uploadBuffer);
	}

	void CommandBufferManager::SubmitForPresent(uint32 imageIndex, VkSemaphore imageAcquiredSemaphore, VkPipelineStageFlags flags)
	{
		TRACY_PROFILE_FUNC();

		VulkanCommandBuffer* commandBuffer = m_PresentCommandBuffers[VulkanRHI::Get()->GetFrameResourcesIndex()];
		VkSemaphore semaphore = m_PresentCmdBufferSemaphores[imageIndex];

		ensuref(commandBuffer->State == CommandBufferState::RecordingCommands);

		if (m_TransferCommandBuffer)
		{
			SubmitTransfer();
		}

#if ATN_ENABLE_TRACY_GPU
		// Collect once per frame before present
		TracyVkCollect(m_TracyContext, commandBuffer->CommandBuffer);
#endif

		commandBuffer->End();

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAcquiredSemaphore;
		submitInfo.pWaitDstStageMask = &flags;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer->CommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &semaphore;

		VK_CHECK(vkQueueSubmit(VulkanRHI::Get()->GetDevice()->GetQueue(), 1, &submitInfo, commandBuffer->SignalFence));
		commandBuffer->State = CommandBufferState::Submitted;
	}

	void CommandBufferManager::NewFrame()
	{
		TRACY_PROFILE_FUNC();

		std::vector<uint32> buffersToDelete;

		for (uint32 i = 0; i < m_CommandBuffers.size(); ++i)
		{
			VulkanCommandBuffer* buffer = m_CommandBuffers[i];

			if (buffer->State == CommandBufferState::Idle)
			{
				if (Time::GetSystemCurrentTime() - buffer->LastUsedTime >= Time::Seconds(COMMAND_BUFFER_DELETE_DELAY))
				{
					buffersToDelete.push_back(i);
				}
			}
			else if (buffer->State == CommandBufferState::Submitted)
			{
				VkResult result = vkGetFenceStatus(Vulkan::GetLogicalDevice(), buffer->SignalFence);

				if (result == VK_SUCCESS)
				{
					buffer->OnCompletion();
				}
			}
		}

		for (uint32 index : buffersToDelete)
		{
			VulkanCommandBuffer* buffer = m_CommandBuffers[index];
			m_CommandBuffers.erase(m_CommandBuffers.begin() + index);
			delete buffer;
		}
	}

	VulkanCommandBuffer* CommandBufferManager::FindFreeCommandBuffer()
	{
		TRACY_PROFILE_FUNC();

		for (uint32 i = 0; i < m_CommandBuffers.size(); ++i)
		{
			VulkanCommandBuffer* buffer = m_CommandBuffers[i];
			if (buffer->State == CommandBufferState::Idle)
			{
				return buffer;
			}
		}

		return AllocateCommandBuffer();
	}

	VulkanCommandBuffer* CommandBufferManager::AllocateCommandBuffer()
	{
		TRACY_PROFILE_FUNC();

		VulkanCommandBuffer* buffer = new VulkanCommandBuffer(m_Pool);
		m_CommandBuffers.push_back(buffer);

		return buffer;
	}
}
