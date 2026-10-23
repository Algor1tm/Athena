#pragma once 

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Format.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanConfiguration.h"
#include "Athena/Renderer/RHI/RHI.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	struct DeviceExtensionsInfo
	{
		bool MemoryBudgetEXTSupported;

		bool HostQueryEXTSupported;
		PFN_vkResetQueryPoolEXT vkResetQueryPoolEXT;

		bool CalibratedTimestampsEXTSupported;
		PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT vkGetPhysicalDeviceCalibrateableTimeDomainsEXT;
		PFN_vkGetCalibratedTimestampsEXT vkGetCalibratedTimestampsEXT;
	};

	struct DeviceFeaturesInfo
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

		const DeviceExtensionsInfo& GetExtensionsInfo() const { return m_ExtensionsInfo; }

		const DeviceFeaturesInfo& GetFeaturesInfo() const { return m_FeaturesInfo; }

		uint32 GetDeviceVulkanVersion() const { return m_DeviceVulkanVersion; }

	private:
		void QueryDeviceExtensions();
		void QueryDeviceFeatures();
		void PostDeviceInit();
		void QueryGPUCapabilities();
		void QueryFormatSupportProperties();

	private:
		VkDevice m_LogicalDevice;
		VkQueue m_Queue;

		DeviceExtensionsInfo m_ExtensionsInfo;
		DeviceFeaturesInfo m_FeaturesInfo;
		VkPhysicalDeviceLimits m_DeviceLimits;
		GPUCapabilities m_GPUCapabilities;

		uint32 m_DeviceVulkanVersion;
		VkPhysicalDevice m_PhysicalDevice;
		uint32 m_QueueFamily;
		std::vector<const char*> m_DeviceExtensions;
	};
}
