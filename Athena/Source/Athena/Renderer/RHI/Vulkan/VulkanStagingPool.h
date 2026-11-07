#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	struct VulkanStagingBuffer
	{
		VulkanBufferAllocation Allocation;
		uint64 Size = 0;
		uint64 RetireSemaphoreValue = 0;
		Time LastTimeUsed;
	};

	class VulkanStagingPool
	{
	public:
		void Clear();

		VulkanBufferAllocation GetBuffer(uint64 size);
		void MarkAsUsed(VulkanBufferAllocation buffer, uint64 retireSemaphoreValue);
		void Update(uint64 semaphoreValue);

	private:
		std::unordered_map<VkBuffer, VulkanStagingBuffer> m_FreeBuffers;
		std::unordered_map<VkBuffer, VulkanStagingBuffer> m_UsedBuffers;
	};
}
