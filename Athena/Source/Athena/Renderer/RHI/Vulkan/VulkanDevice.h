#pragma once 

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Format.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanConfiguration.h"
#include "Athena/Renderer/RHI/RHI.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	struct OptionalDeviceExtensionsInfo
	{
		bool MemoryBudgetEXTSupported;

		bool HostQueryEXTSupported;
		PFN_vkResetQueryPoolEXT vkResetQueryPoolEXT;

#if VULKAN_ENABLE_DEBUG_INFO
		bool DebugMarkerEXTSupported;
		PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBeginEXT;
		PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEndEXT;
		PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsertEXT;
		PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT;
#endif
	};

	struct OptionalDeviceFeaturesInfo
	{
		bool PipelineStatisticsQuery;
		bool SamplerAnisotropy;
		VkPhysicalDeviceHostQueryResetFeatures DeviceHostQueryResetFeatures;
	};

	class VulkanDevice
	{
	public:
		void Initialize(VkInstance instance);

		void Destroy();

		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

		VkDevice GetLogicalDevice() const { return m_LogicalDevice; }

		uint32 GetQueueFamily() const { return m_QueueFamily; }

		VkQueue GetQueue() const { return m_Queue; }

		const VkPhysicalDeviceLimits& GetLimits() const { return m_DeviceLimits; }

		const GPUCapabilities& GetGPUCapabilities() const { return m_GPUCapabilities; }

		const OptionalDeviceExtensionsInfo& GetOptionalExtensionsInfo() const { return m_OptionalExtensionsInfo; }

		const OptionalDeviceFeaturesInfo& GetOptionalFeaturesInfo() const { return m_OptionalFeaturesInfo; }

		uint32 GetDeviceVulkanVersion() const { return m_DeviceVulkanVersion; }

		std::mutex& GetQueueMutex() { return m_QueueMutex; }

		void QueueSubmit(const VkSubmitInfo* submitInfo, VkFence fence);

	private:
		void QueryDeviceExtensions();
		void QueryDeviceFeatures();
		void QueryEXTProcAddresses();
		void QueryGPUCapabilities();

		void MapFormat(Format format, const char* name, bool isColor, bool isDepth, bool isStencil, bool isFloat, bool isSRGB, uint32 bpp, uint32 numComponents);

	private:
		VkDevice m_LogicalDevice;
		VkQueue m_Queue;
		std::mutex m_QueueMutex;

		OptionalDeviceExtensionsInfo m_OptionalExtensionsInfo;
		OptionalDeviceFeaturesInfo m_OptionalFeaturesInfo;
		VkPhysicalDeviceLimits m_DeviceLimits;
		GPUCapabilities m_GPUCapabilities;

		uint32 m_DeviceVulkanVersion;
		VkPhysicalDevice m_PhysicalDevice;
		uint32 m_QueueFamily;
		std::vector<const char*> m_DeviceExtensions;
	};
}
