#pragma once 

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Format.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanCommandContext.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanConfiguration.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanAllocator.h"
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
		bool ShaderInt16;

		VkPhysicalDeviceHostQueryResetFeatures DeviceHostQueryResetFeatures;
		VkPhysicalDevice16BitStorageFeatures Storage16BitFeatures;
		VkPhysicalDeviceShaderFloat16Int8Features Float16Int8Features;
	};

	class VulkanDevice
	{
	public:
		void Initialize(VkInstance instance);

		void Destroy();

		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

		VkDevice GetLogicalDevice() const { return m_LogicalDevice; }

		Ref<VulkanCommandContext> GetGraphicsContext() const { return m_GraphicsContext; }

		Ref<VulkanCommandContext> GetAsyncComputeContext() const { return m_AsyncComputeContext; }

		Ref<VulkanCommandContext> GetAsyncTransferContext() const { return m_AsyncTransferContext; }

		Ref<VulkanAllocator> GetAllocator() const { return m_Allocator; }

		const VkPhysicalDeviceLimits& GetLimits() const { return m_DeviceLimits; }

		const GPUCapabilities& GetGPUCapabilities() const { return m_GPUCapabilities; }

		const DeviceExtensionsInfo& GetExtensionsInfo() const { return m_ExtensionsInfo; }

		const DeviceFeaturesInfo& GetFeaturesInfo() const { return m_FeaturesInfo; }

		uint32 GetDeviceVulkanVersion() const { return m_DeviceVulkanVersion; }

		const VkQueueFamilyProperties& GetQueueFamilyProps(uint32 index) const { return m_QueueFamilyProps[index]; }

		uint32 GetVulkanVersion() const { return m_DeviceVulkanVersion; }

	private:
		void QueryDeviceExtensions();
		void QueryDeviceFeatures();
		void PostDeviceInit();
		void QueryGPUCapabilities();
		void QueryFormatSupportProperties();

	private:
		VkDevice m_LogicalDevice;

		Ref<VulkanCommandContext> m_GraphicsContext;
		Ref<VulkanCommandContext> m_AsyncComputeContext;
		Ref<VulkanCommandContext> m_AsyncTransferContext;
		Ref<VulkanAllocator> m_Allocator;

		DeviceExtensionsInfo m_ExtensionsInfo;
		DeviceFeaturesInfo m_FeaturesInfo;
		VkPhysicalDeviceLimits m_DeviceLimits;
		GPUCapabilities m_GPUCapabilities;

		uint32 m_DeviceVulkanVersion;
		VkPhysicalDevice m_PhysicalDevice;
		std::vector<const char*> m_DeviceExtensions;
		std::vector<VkQueueFamilyProperties> m_QueueFamilyProps;
	};
}
