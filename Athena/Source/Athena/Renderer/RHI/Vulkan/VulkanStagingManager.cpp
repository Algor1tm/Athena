#include "VulkanStagingManager.h"
#include "Athena/Math/Limits.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"


namespace Athena
{
	void VulkanStagingManager::Clear()
	{
		for (auto it = m_FreeBuffers.begin(); it != m_FreeBuffers.end();)
		{
			VulkanRHI::Get()->GetAllocator()->DestroyBuffer(it->second.Allocation);
			it = m_FreeBuffers.erase(it);
		}
	}

	VulkanBufferAllocation VulkanStagingManager::GetBuffer(uint64 size)
	{
		VkBuffer buffer = VK_NULL_HANDLE;

		for (auto it = m_FreeBuffers.begin(); it != m_FreeBuffers.end();)
		{
			if (size == it->second.Size)
			{
				return it->second.Allocation;
			}
		}

		TRACY_PROFILE_SCOPE("VulkanStagingManager::Allocation");

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VulkanBufferAllocation allocation;
		VkResult result = VulkanRHI::Get()->GetAllocator()->AllocateBuffer(&allocation, bufferInfo, 
			VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		if (result != VK_SUCCESS)
		{
			Clear();

			VK_CHECK(VulkanRHI::Get()->GetAllocator()->AllocateBuffer(&allocation, bufferInfo, 
				VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT));
		}

		VulkanStagingBuffer stagingBuffer;
		stagingBuffer.Allocation = allocation;
		stagingBuffer.Size = size;

		m_FreeBuffers[allocation.GetBuffer()] = stagingBuffer;

		return allocation;
	}

	void VulkanStagingManager::MarkAsUsed(VulkanBufferAllocation buffer, uint64 retireSemaphoreValue)
	{
		ensuref(m_FreeBuffers.contains(buffer.GetBuffer()));
		ensuref(!m_UsedBuffers.contains(buffer.GetBuffer()));

		VulkanStagingBuffer stagingBuffer = m_FreeBuffers.at(buffer.GetBuffer());
		stagingBuffer.RetireSemaphoreValue = retireSemaphoreValue;

		m_FreeBuffers.erase(buffer.GetBuffer());
		m_UsedBuffers[buffer.GetBuffer()] = stagingBuffer;
	}

	void VulkanStagingManager::Update(uint64 semaphoreValue)
	{
		for (auto it = m_UsedBuffers.begin(); it != m_UsedBuffers.end();)
		{
			if (it->second.RetireSemaphoreValue <= semaphoreValue)
			{
				it->second.LastTimeUsed = Time::GetSystemCurrentTime();

				m_FreeBuffers.emplace(it->first, std::move(it->second));
				it = m_UsedBuffers.erase(it);
			}
			else
			{
				++it;
			}
		}

		for (auto it = m_FreeBuffers.begin(); it != m_FreeBuffers.end();)
		{
			if (Time::GetSystemCurrentTime() - it->second.LastTimeUsed >= Time::Seconds(VULKAN_STAGING_FREE_DELAY))
			{
				VulkanRHI::Get()->GetAllocator()->DestroyBuffer(it->second.Allocation);
				it = m_FreeBuffers.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
}
