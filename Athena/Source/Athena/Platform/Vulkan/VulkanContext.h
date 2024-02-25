#pragma once

#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/VulkanDevice.h"
#include "Athena/Platform/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>


#define VULKAN_MIN_SUPPORTED_VERSION VK_MAKE_API_VERSION(0, 1, 3, 0)

#define VULKAN_MAX_DEBUG_NAME_LENGTH 30

#ifdef ATN_DEBUG
	#define VULKAN_ENABLE_DEBUG_INFO
#else

#endif


namespace Athena
{
	struct FrameSyncData
	{
		VkSemaphore ImageAcquiredSemaphore;
		VkSemaphore RenderCompleteSemaphore;
		VkFence RenderCompleteFence;
	};

	struct VulkanContextData
	{
		VkInstance Instance;
		VkDebugReportCallbackEXT DebugReport;
		Ref<VulkanAllocator> Allocator;
		Ref<VulkanDevice> Device;
		std::vector<FrameSyncData> FrameSyncData;
		VkCommandPool CommandPool;
	};


	class VulkanContext
	{
	public:
		static void Init();
		static void Shutdown();

		static VkInstance GetInstance() { return s_Data.Instance; }
		static Ref<VulkanAllocator> GetAllocator() { return s_Data.Allocator; }
		static VkCommandPool GetCommandPool() { return s_Data.CommandPool; }

		static Ref<VulkanDevice> GetDevice() { return s_Data.Device; }
		static VkDevice GetLogicalDevice() { return s_Data.Device->GetLogicalDevice(); }
		static VkPhysicalDevice GetPhysicalDevice() { return s_Data.Device->GetPhysicalDevice(); }

		static const FrameSyncData& GetFrameSyncData(uint32 frameIndex) { return s_Data.FrameSyncData[frameIndex]; }

	private:
		static VulkanContextData s_Data;
	};
}
