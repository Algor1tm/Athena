#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RendererAPI.h"
#include "Athena/Platform/Vulkan/VulkanDevice.h"

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
		static Ref<VulkanDevice> GetCurrentDevice() { return s_CurrentDevice; }
		static const FrameSyncData& GetFrameSyncData(uint32 frameIndex) { return s_FrameSyncData[frameIndex]; }
		static VkCommandPool GetCommandPool() { return s_CommandPool; }

	private:
		static VkInstance s_Instance;
		static VkAllocationCallbacks* s_Allocator;
		static Ref<VulkanDevice> s_CurrentDevice;
		static std::vector<FrameSyncData> s_FrameSyncData;
		static VkCommandPool s_CommandPool;
	};


	class VulkanRenderer: public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual void Submit(const Ref<CommandBuffer>& cmdBuff) override;

	private:
		VkDebugReportCallbackEXT m_DebugReport;
	};
}
