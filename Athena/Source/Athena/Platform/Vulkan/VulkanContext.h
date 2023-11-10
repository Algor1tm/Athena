#pragma once

#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/VulkanDevice.h"
#include "Athena/Platform/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>


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
		Ref<VulkanDevice> CurrentDevice;
		std::vector<FrameSyncData> FrameSyncData;
		VkCommandPool CommandPool;
		VkCommandBuffer ActiveCommandBuffer;
		VkDescriptorPool DescriptorPool;
	};


	class VulkanContext
	{
	public:
		static void Init();
		static void Shutdown();

		static VkInstance GetInstance() { return s_Data.Instance; }
		static Ref<VulkanAllocator> GetAllocator() { return s_Data.Allocator; }

		static Ref<VulkanDevice> GetDevice() { return s_Data.CurrentDevice; }
		static VkDevice GetLogicalDevice() { return s_Data.CurrentDevice->GetLogicalDevice(); }
		static VkPhysicalDevice GetPhysicalDevice() { return s_Data.CurrentDevice->GetPhysicalDevice(); }

		static const FrameSyncData& GetFrameSyncData(uint32 frameIndex) { return s_Data.FrameSyncData[frameIndex]; }

		static VkCommandPool GetCommandPool() { return s_Data.CommandPool; }
		static void SetActiveCommandBuffer(VkCommandBuffer commandBuffer) { s_Data.ActiveCommandBuffer = commandBuffer; }
		static VkCommandBuffer GetActiveCommandBuffer() { return s_Data.ActiveCommandBuffer; }

		static VkDescriptorPool GetDescriptorPool() { return s_Data.DescriptorPool; }

	private:
		static VulkanContextData s_Data;
	};
}
