#include "VulkanDevice.h"

#include "Athena/Core/ConsoleManager.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanPlatform.h"

namespace Athena
{
	static const char* s_RequiredVulkanDeviceExtensions[] =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME, // discard pixel
		VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
		VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME, 

		nullptr
	};

	static const char* s_OptionalVulkanDeviceExtensions[] =
	{
		VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
		VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME,
		VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,

		nullptr
	};

	static bool IsExtensionPresent(const std::vector<const char*>& extensions, const char* extension)
	{
		for (uint32 j = 0; j < extensions.size(); ++j)
		{
			if (!strcmp(extension, extensions[j]))
			{
				return true;
			}
		}

		return false;
	}

	static const char* GetVendorName(uint32 vendorID)
	{
		if (vendorID == 0x10DE)
			return "NVIDIA";
		else if (vendorID == 0x1002)
			return "AMD";
		else if (vendorID == 0x8086)
			return "Intel";
		else if (vendorID == 0x1414)
			return "WARP";
		else if (vendorID == 0x13B5)
			return "ARM (Mali)";
		else if (vendorID == 0x5143)
			return "Qualcomm (Adreno)";
		else if (vendorID == 0x1010)
			return "Imagination Technologies";

		return "Unknown";
	}

	static String DecodeDriverVersion(const char* vendor, uint32 driverVersion)
	{
		String result;

		if (!strcmp(vendor, "NVIDIA"))
		{
			uint32_t nvidiaMajor = (driverVersion >> 22) & 0x3FF;
			uint32_t nvidiaMinor = (driverVersion >> 14) & 0xFF;
			uint32_t nvidiaBuild = (driverVersion >> 6) & 0xFF;
			uint32_t nvidiaRevision = driverVersion & 0x3F;

			result = fmt::format("{}.{}.{}.{}", nvidiaMajor, nvidiaMinor, nvidiaBuild, nvidiaRevision);
		}
		else if (!strcmp(vendor, "AMD"))
		{
			uint32_t amdYear = (driverVersion >> 22) & 0x3FF;
			uint32_t amdMonth = (driverVersion >> 14) & 0xFF;
			uint32_t amdPatch = driverVersion & 0x3FFF;

			result = fmt::format("{}.{}.{}", amdYear, amdMonth, amdPatch);
		}
		else if (!strcmp(vendor, "Intel"))
		{
			uint32_t intelMajor = (driverVersion >> 16);
			uint32_t intelBuild = driverVersion & 0xFFFF;

			result = fmt::format("{}.{}", intelMajor, intelBuild);
		}
		else
		{
			result = std::to_string(driverVersion);
		}

		return result;
	}

	static const char* VkPhysicalDeviceTypeToString(VkPhysicalDeviceType type)
	{
		switch (type)
		{
		case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_OTHER:			return "Other";
		case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:	return "Integrated";
		case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:	return "Discrete";
		case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:		return "Virtual";
		case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_CPU:				return "CPU";
		}

		return "";
	}

	void VulkanDevice::Initialize(VkInstance instance)
	{
		// Select GPU
		{
			uint32 gpuCount;
			VK_CHECK(vkEnumeratePhysicalDevices(instance, &gpuCount, NULL));

			std::vector<VkPhysicalDevice> gpus(gpuCount);
			VK_CHECK(vkEnumeratePhysicalDevices(instance, &gpuCount, gpus.data()));

			String message = "Avaialble GPUs: \n\t";
			uint32 selectedGPUInex = 0;

			for (uint32 i = 0; i < gpuCount; i++)
			{
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(gpus[i], &properties);
				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					selectedGPUInex = i;
				}

				message += fmt::format("{}\n\t", properties.deviceName);
			}

			ATN_LOG_INFO(Vulkan, message);

			message = fmt::format("Selected GPU properties: \n\t");

			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(gpus[selectedGPUInex], &properties);

			uint32 deviceVariant = VK_API_VERSION_VARIANT(properties.apiVersion);
			uint32 deviceMajor = VK_API_VERSION_MAJOR(properties.apiVersion);
			uint32 deviceMinor = VK_API_VERSION_MINOR(properties.apiVersion);
			uint32 devicePatch = VK_API_VERSION_PATCH(properties.apiVersion);

			m_DeviceVulkanVersion = properties.apiVersion;

			const char* vendorName = GetVendorName(properties.vendorID);

			message += fmt::format("API version: {}.{}.{}.{} \n\t", deviceVariant, deviceMajor, deviceMinor, devicePatch);
			message += fmt::format("Driver version: {} \n\t", DecodeDriverVersion(vendorName, properties.driverVersion));
			message += fmt::format("Vendor Name: {} \n\t", vendorName);
			message += fmt::format("Device ID: {} \n\t", properties.deviceID);
			message += fmt::format("Device Type: {} \n\t", VkPhysicalDeviceTypeToString(properties.deviceType));
			message += fmt::format("Device Name: {} \n\t", properties.deviceName);

			ATN_LOG_INFO(Vulkan, message);

			m_PhysicalDevice = gpus[selectedGPUInex];
		}

		struct QueueInfo
		{
			uint32 QueueFamily = UINT32_MAX;
			uint32 QueueIndex = 0;
		};

		struct QueueFamilyInfo
		{
			uint32 Count = 0;
			std::vector<float> Priorities;
		};

		struct QueuesProperties
		{
			QueueInfo Graphics;
			QueueInfo Compute;
			QueueInfo Transfer;

			std::vector<QueueFamilyInfo> QueueFamilies;
		};

		QueuesProperties queues;

		// Select graphics queue family
		{
			uint32 count;
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, NULL);

			m_QueueFamilyProps.resize(count);
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, m_QueueFamilyProps.data());

			String message = "Queue Families: \n\t";

			for (uint32 i = 0; i < count; i++)
			{
				String flags;

				flags += m_QueueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ? "Graphics, " : "";
				flags += m_QueueFamilyProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT ? "Compute, " : "";
				flags += m_QueueFamilyProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT ? "Transfer, " : "";
				flags += m_QueueFamilyProps[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? "SparseBinding, " : "";

				message += fmt::format("{}: {} timestamps = {}, count = {}\n\t", i, flags, m_QueueFamilyProps[i].timestampValidBits > 0, m_QueueFamilyProps[i].queueCount);
			}

			ATN_LOG_INFO(Vulkan, message);

			queues.QueueFamilies.resize(m_QueueFamilyProps.size());

			auto findQueue = [this, &queues](VkQueueFlags flags, float priority) -> QueueInfo
			{
				uint32 bestFamily = UINT32_MAX;
				uint32 bestScore = UINT32_MAX;

				for (uint32 i = 0; i < m_QueueFamilyProps.size(); ++i)
				{
					const auto& family = m_QueueFamilyProps[i];

					if (family.queueCount == 0 || queues.QueueFamilies[i].Count >= family.queueCount)
						continue;

					if ((family.queueFlags & flags) != flags)
						continue;

					const uint32 extraFlags = uint32(family.queueFlags & ~flags);
					const uint32 score = std::popcount(extraFlags);

					if (score < bestScore)
					{
						bestScore = score;
						bestFamily = i;
					}
				}

				if (bestFamily == UINT32_MAX)
					return { UINT32_MAX, 0 };

				const uint32 queueIndex = queues.QueueFamilies[bestFamily].Count++;
				queues.QueueFamilies[bestFamily].Priorities.push_back(priority);

				return { bestFamily, queueIndex };
			};

			queues.Graphics = findQueue(VK_QUEUE_GRAPHICS_BIT, 1.f);

			bool enableAsyncCompute = ConsoleManager::Get().FindCVar("r.EnableAsyncCompute")->GetBool();
			if(enableAsyncCompute)
				queues.Compute = findQueue(VK_QUEUE_COMPUTE_BIT, 1.f);

			bool enableAsyncTransfer = ConsoleManager::Get().FindCVar("r.EnableAsyncTransfer")->GetBool();
			if(enableAsyncTransfer)
				queues.Transfer = findQueue(VK_QUEUE_TRANSFER_BIT, 1.f);
		}

		// Create Logical Device
		{
			const float queuePriority[] = { 1.0f };

			std::vector<VkDeviceQueueCreateInfo> queueInfos;

			for (uint32 i = 0; i < queues.QueueFamilies.size(); ++i)
			{
				if (queues.QueueFamilies[i].Count == 0)
					continue;

				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = i;
				queueCreateInfo.queueCount = queues.QueueFamilies[i].Count;
				queueCreateInfo.pQueuePriorities = queues.QueueFamilies[i].Priorities.data();

				queueInfos.push_back(queueCreateInfo);
			}

			QueryDeviceExtensions();
			QueryDeviceFeatures();

			VkPhysicalDeviceHostQueryResetFeaturesEXT hostQueryResetFeatures = m_FeaturesInfo.DeviceHostQueryResetFeatures;

			VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT demoteToHelperInvocationFeatures = {};
			demoteToHelperInvocationFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT;
			demoteToHelperInvocationFeatures.shaderDemoteToHelperInvocation = VK_TRUE;
			demoteToHelperInvocationFeatures.pNext = &hostQueryResetFeatures;

			VkPhysicalDeviceSynchronization2Features synchronization2Features{};
			synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
			synchronization2Features.synchronization2 = VK_TRUE;
			synchronization2Features.pNext = &demoteToHelperInvocationFeatures;

			VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{};
			timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
			timelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;
			timelineSemaphoreFeatures.pNext = &synchronization2Features;

			void* deviceFeaturesChain = &timelineSemaphoreFeatures;

			VkPhysicalDeviceFeatures deviceFeatures = {};
			deviceFeatures.geometryShader = VK_TRUE;
			deviceFeatures.wideLines = VK_TRUE;
			deviceFeatures.pipelineStatisticsQuery = m_FeaturesInfo.PipelineStatisticsQuery;
			deviceFeatures.samplerAnisotropy = m_FeaturesInfo.SamplerAnisotropy;
			deviceFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE;

			VkDeviceCreateInfo deviceCI = {};
			deviceCI.pNext = deviceFeaturesChain;
			deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceCI.queueCreateInfoCount = queueInfos.size();
			deviceCI.pQueueCreateInfos = queueInfos.data();
			deviceCI.enabledExtensionCount = m_DeviceExtensions.size();
			deviceCI.ppEnabledExtensionNames = m_DeviceExtensions.data();
			deviceCI.pEnabledFeatures = &deviceFeatures;

			VkResult result = vkCreateDevice(m_PhysicalDevice, &deviceCI, nullptr, &m_LogicalDevice);
			if (result != 0)
			{
				ATN_LOG_FATAL(Vulkan, "Failed to create vulkan device, try to update drivers or use DirectX instead!");
			}

			PostDeviceInit();
		}

		{
			m_GraphicsContext = Ref<VulkanCommandContext>::Create(queues.Graphics.QueueFamily, CommandContextType::Graphics);

			if (queues.Compute.QueueFamily != UINT32_MAX)
			{
				m_AsyncComputeContext = Ref<VulkanCommandContext>::Create(queues.Compute.QueueFamily, CommandContextType::AsyncCompute);
			}
			else
			{
				m_AsyncComputeContext = m_GraphicsContext;
			}

			if (queues.Transfer.QueueFamily != UINT32_MAX)
			{
				m_AsyncTransferContext = Ref<VulkanCommandContext>::Create(queues.Transfer.QueueFamily, CommandContextType::AsyncTransfer);
			}
			else
			{
				m_AsyncTransferContext = m_AsyncComputeContext;
			}
		}
	}

	void VulkanDevice::Destroy()
	{
		m_GraphicsContext.Release();
		m_AsyncComputeContext.Release();
		m_AsyncTransferContext.Release();

		vkDestroyDevice(m_LogicalDevice, nullptr);
	}

	void VulkanDevice::PostDeviceInit()
	{
		m_ExtensionsInfo.MemoryBudgetEXTSupported = IsExtensionPresent(m_DeviceExtensions, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);

		m_ExtensionsInfo.HostQueryEXTSupported = IsExtensionPresent(m_DeviceExtensions, VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
		if (m_ExtensionsInfo.HostQueryEXTSupported)
		{
			m_ExtensionsInfo.vkResetQueryPoolEXT = Vulkan::GetDeviceProc<PFN_vkResetQueryPoolEXT>("vkResetQueryPoolEXT");
		}

		m_ExtensionsInfo.CalibratedTimestampsEXTSupported = IsExtensionPresent(m_DeviceExtensions, VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME);
		if (m_ExtensionsInfo.CalibratedTimestampsEXTSupported)
		{
			m_ExtensionsInfo.vkGetPhysicalDeviceCalibrateableTimeDomainsEXT = Vulkan::GetInstanceProc<PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT>("vkGetPhysicalDeviceCalibrateableTimeDomainsEXT");
			m_ExtensionsInfo.vkGetCalibratedTimestampsEXT = Vulkan::GetDeviceProc<PFN_vkGetCalibratedTimestampsEXT>("vkGetCalibratedTimestampsEXT");
		}

		QueryGPUCapabilities();
	}

	void VulkanDevice::QueryDeviceExtensions()
	{
		uint32 supportedExtensionCount = 0;
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, NULL, &supportedExtensionCount, nullptr);

		std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, NULL, &supportedExtensionCount, supportedExtensions.data());

		String message = "Found vulkan device extensions: \n\t";
		for (auto ext : supportedExtensions)
			message += fmt::format("'{}'\n\t", ext.extensionName);

		ATN_LOG_INFO(Vulkan, message);

		std::vector<const char*> missingExtensions;
		std::vector<const char*> platformExtensions = VulkanPlatform::GetRequiredDeviceExtensions();

		for (uint32 i = 0; i < ATN_ARRAY_SIZE(s_RequiredVulkanDeviceExtensions); ++i)
		{
			if (!s_RequiredVulkanDeviceExtensions[i])
				continue;

			bool found = false;
			for (uint32 j = 0; j < supportedExtensions.size(); ++j)
			{
				if (!strcmp(s_RequiredVulkanDeviceExtensions[i], supportedExtensions[j].extensionName))
				{
					found = true;
					break;
				}
			}

			if (found)
				m_DeviceExtensions.push_back(s_RequiredVulkanDeviceExtensions[i]);
			else
				missingExtensions.push_back(s_RequiredVulkanDeviceExtensions[i]);
		}


		for (uint32 i = 0; i < platformExtensions.size(); ++i)
		{
			if (!platformExtensions[i])
				continue;

			bool found = false;
			for (uint32 j = 0; j < supportedExtensions.size(); ++j)
			{
				if (!strcmp(platformExtensions[i], supportedExtensions[j].extensionName))
				{
					found = true;
					break;
				}
			}

			if (found)
				m_DeviceExtensions.push_back(platformExtensions[i]);
			else
				missingExtensions.push_back(platformExtensions[i]);
		}


		for (uint32 i = 0; i < ATN_ARRAY_SIZE(s_OptionalVulkanDeviceExtensions); ++i)
		{
			if (!s_OptionalVulkanDeviceExtensions[i])
				continue;

			bool found = false;
			for (uint32 j = 0; j < supportedExtensions.size(); ++j)
			{
				if (s_OptionalVulkanDeviceExtensions[i] && !strcmp(s_OptionalVulkanDeviceExtensions[i], supportedExtensions[j].extensionName))
				{
					found = true;
					break;
				}
			}

			if (found)
				m_DeviceExtensions.push_back(s_OptionalVulkanDeviceExtensions[i]);
		}

		if (!missingExtensions.empty())
		{
			message = "Missing device extensions: \n\t";
			for (auto ext : missingExtensions)
				message += fmt::format("'{}'\n\t", ext);

			ATN_LOG_FATAL(Vulkan, message);
		}

		message = "Vulkan using device extensions: \n\t";
		for (auto ext : m_DeviceExtensions)
			message += fmt::format("'{}'\n\t", ext);

		ATN_LOG_INFO(Vulkan, message);
	}

	void VulkanDevice::QueryDeviceFeatures()
	{
		VkPhysicalDeviceHostQueryResetFeatures hostQueryResetFeatures = {};
		hostQueryResetFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;

		VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT demoteFeatures = {};
		demoteFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT;
		demoteFeatures.pNext = &hostQueryResetFeatures;

		VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.pNext = &demoteFeatures;

		vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &deviceFeatures2);
		
		bool requiredFeaturesSupported =
			deviceFeatures2.features.geometryShader &&
			deviceFeatures2.features.shaderStorageImageWriteWithoutFormat &&
			demoteFeatures.shaderDemoteToHelperInvocation;

		ensure(requiredFeaturesSupported, "Vulkan physical device missing features!");

		m_FeaturesInfo.PipelineStatisticsQuery = deviceFeatures2.features.pipelineStatisticsQuery;
		m_FeaturesInfo.SamplerAnisotropy = deviceFeatures2.features.samplerAnisotropy;
		m_FeaturesInfo.DeviceHostQueryResetFeatures = hostQueryResetFeatures;
	}

	void VulkanDevice::QueryGPUCapabilities()
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);

		m_DeviceLimits = deviceProperties.limits;

		m_GPUCapabilities.Name = deviceProperties.deviceName;

		VkPhysicalDeviceMemoryProperties memoryProps;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProps);

		m_GPUCapabilities.VRAM = 0;
		for (uint32 i = 0; i < memoryProps.memoryHeapCount; ++i)
		{
			if (memoryProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
				m_GPUCapabilities.VRAM += memoryProps.memoryHeaps[i].size / 1024; // KBs
		}

		m_GPUCapabilities.MaxTextureDimension2D = m_DeviceLimits.maxImageDimension2D;
		m_GPUCapabilities.MaxTextureDimensionCube = m_DeviceLimits.maxImageDimensionCube;
		m_GPUCapabilities.MaxFramebufferWidth = m_DeviceLimits.maxFramebufferWidth;
		m_GPUCapabilities.MaxFramebufferHeight = m_DeviceLimits.maxFramebufferHeight;
		m_GPUCapabilities.MaxViewportWidth = m_DeviceLimits.maxViewportDimensions[0];
		m_GPUCapabilities.MaxViewportHeight = m_DeviceLimits.maxViewportDimensions[1];
		m_GPUCapabilities.MaxSamplerLodBias = m_DeviceLimits.maxSamplerLodBias;
		m_GPUCapabilities.MaxSamplerAnisotropy = m_DeviceLimits.maxSamplerAnisotropy;

		QueryFormatSupportProperties();
	}

	void VulkanDevice::QueryFormatSupportProperties()
	{
		for (uint32 i = 0; i < ATN_ARRAY_SIZE(m_GPUCapabilities.Formats); ++i)
		{
			Format format = Format(i);
			FormatSupportProperties& properties = m_GPUCapabilities.Formats[format];

			if (format == Format::Unknown || format == Format::FormatNum)
			{
				properties.IsTextureSupported = false;
				properties.IsRenderTargetSupported = false;
				properties.IsVertexBufferSupported = false;
			}
			else
			{
				VkFormatProperties props;
				vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, Vulkan::TranslateFormat(format), &props);

				VkFormatFeatureFlags textureRequiredFlags = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
				VkFormatFeatureFlags renderTargetRequiredFlags = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
				VkFormatFeatureFlags depthTargetRequiredFlags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
				VkFormatFeatureFlags bufferRequiredFlags = VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT;

				properties.IsTextureSupported = (props.optimalTilingFeatures & textureRequiredFlags) == textureRequiredFlags;

				properties.IsRenderTargetSupported = (props.optimalTilingFeatures & renderTargetRequiredFlags) == renderTargetRequiredFlags ||
					(props.optimalTilingFeatures & depthTargetRequiredFlags) == depthTargetRequiredFlags;

				properties.IsVertexBufferSupported = (props.bufferFeatures & bufferRequiredFlags) == bufferRequiredFlags;
			}
		}
	}
}
