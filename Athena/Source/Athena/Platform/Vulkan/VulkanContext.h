#pragma once

#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/VulkanDevice.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	struct FrameSyncData
	{
		VkSemaphore ImageAcquiredSemaphore;
		VkSemaphore RenderCompleteSemaphore;
		VkFence RenderCompleteFence;
	};


	class VulkanContext
	{
	public:
		friend class VulkanRenderer;

	public:
		static VkInstance GetInstance() { return s_Instance; }
		static VkAllocationCallbacks* GetAllocator() { return s_Allocator; }

		static Ref<VulkanDevice> GetDevice() { return s_CurrentDevice; }
		static VkDevice GetLogicalDevice() { return s_CurrentDevice->GetLogicalDevice(); }
		static VkPhysicalDevice GetPhysicalDevice() { return s_CurrentDevice->GetPhysicalDevice(); }

		static const FrameSyncData& GetFrameSyncData(uint32 frameIndex) { return s_FrameSyncData[frameIndex]; }

		static VkCommandPool GetCommandPool() { return s_CommandPool; }
		static void SetActiveCommandBuffer(VkCommandBuffer commandBuffer) { s_ActiveCommandBuffer = commandBuffer; }
		static VkCommandBuffer GetActiveCommandBuffer() { return s_ActiveCommandBuffer; }

	private:
		static VkInstance s_Instance;
		static VkAllocationCallbacks* s_Allocator;
		static Ref<VulkanDevice> s_CurrentDevice;
		static std::vector<FrameSyncData> s_FrameSyncData;
		static VkCommandPool s_CommandPool;
		static VkCommandBuffer s_ActiveCommandBuffer;
	};
}
