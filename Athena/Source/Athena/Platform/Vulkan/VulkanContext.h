#pragma once

#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/VulkanDevice.h"
#include "Athena/Platform/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>


#define VULKAN_MAX_DEBUG_NAME_LENGTH 50

#if ATN_DIST
	#define VULKAN_ENABLE_DEBUG_INFO 0 
	#define VULKAN_ENABLE_MEMORY_DEBUG_INFO 0
#else
	#define VULKAN_ENABLE_DEBUG_INFO 1
	#define VULKAN_ENABLE_MEMORY_DEBUG_INFO 0
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
		uint32 InstanceVersion;
		VkDebugReportCallbackEXT DebugReport;
		Ref<VulkanAllocator> Allocator;
		Ref<DescriptorSetAllocator> DescriptorSetAllocator;
		Ref<VulkanDevice> Device;
		std::vector<FrameSyncData> FrameSyncData;
		VkCommandPool CommandPool;
		VkPipelineLayout BindedPipelineLayout;
	};


	class VulkanContext
	{
	public:
		static void Init();
		static void Shutdown();

		static VkInstance GetInstance() { return s_Data.Instance; }
		static uint32 GetInstanceVersion() { return s_Data.InstanceVersion; }
		static Ref<VulkanAllocator> GetAllocator() { return s_Data.Allocator; }
		static VkCommandPool GetCommandPool() { return s_Data.CommandPool; }
		static Ref<DescriptorSetAllocator> GetDescriptorSetAllocator() { return s_Data.DescriptorSetAllocator; }

		static Ref<VulkanDevice> GetDevice() { return s_Data.Device; }
		static VkDevice GetLogicalDevice() { return s_Data.Device->GetLogicalDevice(); }
		static VkPhysicalDevice GetPhysicalDevice() { return s_Data.Device->GetPhysicalDevice(); }

		static const FrameSyncData& GetFrameSyncData(uint32 frameIndex) { return s_Data.FrameSyncData[frameIndex]; }

		static void BindPipelineLayout(VkPipelineLayout layout) { s_Data.BindedPipelineLayout = layout; }
		static VkPipelineLayout GetBindedPipelineLayout() { return s_Data.BindedPipelineLayout; }

	private:
		static VulkanContextData s_Data;
	};
}
