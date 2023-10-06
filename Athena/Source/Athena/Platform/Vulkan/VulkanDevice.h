#pragma once

#include "Athena/Core/Core.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanDevice
	{
	public:
		VulkanDevice();
		~VulkanDevice();

		void WaitIdle();

		VkPhysicalDevice GetPhysicalDevice() { return m_PhysicalDevice; }
		VkDevice GetLogicalDevice() { return m_LogicalDevice; }
		uint32 GetQueueFamily() { return m_QueueFamily; }
		VkQueue GetQueue() { return m_Queue; }

	private:
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_LogicalDevice;
		uint32 m_QueueFamily;
		VkQueue m_Queue;
	};
}
