#include "CommandBufferManager.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"

#define COMMAND_BUFFER_DELETE_DELAY 1 // seconds


namespace Athena
{
	VulkanCommandBuffer::VulkanCommandBuffer(VkCommandPool pool)
		: State(CommandBufferState::Idle),
		OnCompletionCallbacks(10 * 1024), // 10Kb
		LastIdleTime(0)
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


	void CommandBufferManager::Init()
	{
		VkCommandPoolCreateInfo commandPoolCI = {};
		commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCI.queueFamilyIndex = VulkanRHI::Get()->GetDevice()->GetQueueFamily();
		commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK(vkCreateCommandPool(Vulkan::GetLogicalDevice(), &commandPoolCI, nullptr, &m_Pool));
	}

	void CommandBufferManager::Destroy()
	{
		vkDestroyCommandPool(Vulkan::GetLogicalDevice(), m_Pool, nullptr);
	}

	VulkanCommandBuffer* CommandBufferManager::GetCommandBuffer()
	{
		VulkanCommandBuffer* buffer = FindFreeCommandBuffer();
		buffer->Begin();

		return buffer;
	}

	VulkanCommandBuffer* CommandBufferManager::GetUploadCommandBuffer()
	{
		if (!m_UploadCommandBuffer)
		{
			m_UploadCommandBuffer = FindFreeCommandBuffer();
			m_UploadCommandBuffer->Begin();
		}

		return m_UploadCommandBuffer;
	}

	void CommandBufferManager::Submit(VulkanCommandBuffer* commandBuffer, bool wait)
	{
		ensuref(m_UploadCommandBuffer != commandBuffer && commandBuffer != nullptr);
		ensuref(commandBuffer->State == CommandBufferState::RecordingCommands);

		if (m_UploadCommandBuffer)
		{
			SubmitUpload();
		}

		commandBuffer->End();

		VkCommandBuffer buffers[1] = { commandBuffer->CommandBuffer };

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = ATN_ARRAY_SIZE(buffers);
		submitInfo.pCommandBuffers = buffers;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		if (wait)
		{
			VulkanRHI::Get()->GetDevice()->QueueSubmit(&submitInfo, commandBuffer->SignalFence);
			VK_CHECK(vkWaitForFences(Vulkan::GetLogicalDevice(), 1, &commandBuffer->SignalFence, VK_TRUE, VULKAN_DEFAULT_FENCE_TIMEOUT));
			OnCompletion(commandBuffer);
		}
		else
		{
			VulkanRHI::Get()->GetDevice()->QueueSubmit(&submitInfo, VK_NULL_HANDLE);
			commandBuffer->State = CommandBufferState::Submitted;
		}

	}

	void CommandBufferManager::SubmitUpload()
	{
		if (!m_UploadCommandBuffer)
			return;

		// Insert memory barrier between upload commands and rendering commands
		VkMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;  
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;

		vkCmdPipelineBarrier(
			m_UploadCommandBuffer->CommandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			1, &memoryBarrier,
			0, nullptr,
			0, nullptr
		);

		VulkanCommandBuffer* uploadBuffer = m_UploadCommandBuffer;
		m_UploadCommandBuffer = nullptr;

		Submit(uploadBuffer);
	}

	void CommandBufferManager::OnUpdate()
	{
		std::vector<uint32> buffersToDelete;

		for (uint32 i = 0; i < m_CommandBuffers.size(); ++i)
		{
			VulkanCommandBuffer* buffer = m_CommandBuffers[i];

			if (buffer->State == CommandBufferState::Idle)
			{
				if (buffer->LastIdleTime >= Time::Seconds(COMMAND_BUFFER_DELETE_DELAY))
				{
					buffersToDelete.push_back(i);
				}
			}
			else if (buffer->State == CommandBufferState::Submitted)
			{
				VkResult result = vkGetFenceStatus(Vulkan::GetLogicalDevice(), buffer->SignalFence);

				if (result == VK_SUCCESS)
				{
					OnCompletion(buffer);
				}
			}
		}

		for (uint32 index : buffersToDelete)
		{
			VulkanCommandBuffer* buffer = *m_CommandBuffers.erase(m_CommandBuffers.begin() + index);
			delete buffer;
		}
	}

	void CommandBufferManager::OnCompletion(VulkanCommandBuffer* commandBuffer)
	{ 
		vkResetFences(Vulkan::GetLogicalDevice(), 1, &commandBuffer->SignalFence);

		commandBuffer->OnCompletionCallbacks.Flush();
		commandBuffer->State = CommandBufferState::Idle;
		commandBuffer->LastIdleTime = Time::GetCurrentTime();
	}

	VulkanCommandBuffer* CommandBufferManager::FindFreeCommandBuffer()
	{
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
		VulkanCommandBuffer* buffer = new VulkanCommandBuffer(m_Pool);
		m_CommandBuffers.push_back(buffer);

		return buffer;
	}
}
