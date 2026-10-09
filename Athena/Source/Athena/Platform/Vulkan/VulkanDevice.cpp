#include "VulkanDevice.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanDevice::VulkanDevice()
	{
		// Select GPU
		{
			uint32 gpuCount;
			VK_CHECK(vkEnumeratePhysicalDevices(VulkanContext::GetInstance(), &gpuCount, NULL));

			std::vector<VkPhysicalDevice> gpus(gpuCount);

			VK_CHECK(vkEnumeratePhysicalDevices(VulkanContext::GetInstance(), &gpuCount, gpus.data()));

			String message = "GPUs: \n\t";
			String selectedGPUName;

			uint32 useGpu = 0;
			for (uint32 i = 0; i < gpuCount; i++)
			{
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(gpus[i], &properties);
				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					useGpu = i;
					selectedGPUName = properties.deviceName;
				}

				message += fmt::format("{}\n\t", properties.deviceName);
			}

			ATN_LOG_INFO(Vulkan, message);
			ATN_LOG_INFO(Vulkan, "Selected GPU: {}\n", selectedGPUName);

			m_PhysicalDevice = gpus[useGpu];
		};

		
		// Select graphics queue family
		{
			uint32 count;
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, NULL);

			std::vector<VkQueueFamilyProperties> queues(count);
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, queues.data());

			String message = "Queue Families: \n\t";

			m_QueueFamily = UINT32_MAX;
			VkQueueFlagBits requestedQueueFlags = VkQueueFlagBits(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
			for (uint32 i = 0; i < count; i++)
			{
				// Select fisrt queue that support requested flags
				bool supported = true;
				supported = supported && queues[i].queueFlags & requestedQueueFlags;
				supported = supported && queues[i].timestampValidBits > 0;
				if (supported && (m_QueueFamily == UINT32_MAX))
					m_QueueFamily = i;

				String flags;

				flags += queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ? "Graphics, " : "";
				flags += queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT ? "Compute, " : "";
				flags += queues[i].queueFlags & VK_QUEUE_TRANSFER_BIT ? "Transfer, " : "";
				flags += queues[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? "SparseBinding, " : "";

				message += fmt::format("{}: {} timestamps = {}, count = {}\n\t", i, flags, queues[i].timestampValidBits > 0, queues[i].queueCount);
			}

			ATN_LOG_INFO(Vulkan, message);
			ensure(m_QueueFamily != UINT32_MAX, "Failed to find queue family that supports VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT operations and timestamps");
		};

		// Create Logical Device
		{
			String message = "Queues created: \n\t";

			const float queuePriority[] = { 1.0f };

			VkDeviceQueueCreateInfo queueCIs[1] = {};
			queueCIs[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCIs[0].queueFamilyIndex = m_QueueFamily;
			queueCIs[0].queueCount = 1;
			queueCIs[0].pQueuePriorities = queuePriority;

			message += fmt::format("QueueFamily - {}, count - {}\n\t", m_QueueFamily, 1);
			ATN_LOG_INFO(Vulkan, message);

			std::vector<const char*> deviceExtensions = { 
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME, // discard/clip
				"VK_EXT_memory_budget" };

			if (VK_VERSION_MINOR(VulkanContext::GetInstanceVersion()) < 2)
			{
				deviceExtensions.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
			}

#if VULKAN_ENABLE_DEBUG_INFO
			deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
#endif

			CheckEnabledExtensions(deviceExtensions);

			// GPU profiling
			VkPhysicalDeviceHostQueryResetFeaturesEXT hostQueryResetFeatures = {};
			hostQueryResetFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT;
			hostQueryResetFeatures.hostQueryReset = VK_TRUE;
			hostQueryResetFeatures.pNext = nullptr;

			// discard/clip features
			VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT demoteToHelperInvocationFeatures = {};
			demoteToHelperInvocationFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT;
			demoteToHelperInvocationFeatures.shaderDemoteToHelperInvocation = true;
			demoteToHelperInvocationFeatures.pNext = &hostQueryResetFeatures;

			void* deviceFeaturesChain = &demoteToHelperInvocationFeatures;

			VkPhysicalDeviceFeatures deviceFeatures = {};
			deviceFeatures.geometryShader = VK_TRUE;
			deviceFeatures.wideLines = VK_TRUE;
			deviceFeatures.pipelineStatisticsQuery = VK_TRUE;
			deviceFeatures.samplerAnisotropy = VK_TRUE;
			deviceFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE;

			CheckSupportedFeatures();

			VkDeviceCreateInfo deviceCI = {};
			deviceCI.pNext = deviceFeaturesChain;
			deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceCI.queueCreateInfoCount = std::size(queueCIs);
			deviceCI.pQueueCreateInfos = queueCIs;
			deviceCI.enabledExtensionCount = deviceExtensions.size();
			deviceCI.ppEnabledExtensionNames = deviceExtensions.data();
			deviceCI.pEnabledFeatures = &deviceFeatures;

			VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCI, nullptr, &m_LogicalDevice));
			vkGetDeviceQueue(m_LogicalDevice, m_QueueFamily, 0, &m_Queue);
		};
	}

	VulkanDevice::~VulkanDevice()
	{
		vkDestroyDevice(m_LogicalDevice, nullptr);
	}

	void VulkanDevice::GetDeviceCapabilities(RenderCapabilities& deviceCaps) const
	{
		VkPhysicalDeviceMemoryProperties memoryProps;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProps);

		deviceCaps.VRAM = 0;
		for (uint32 i = 0; i < memoryProps.memoryHeapCount; ++i)
		{
			if(memoryProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
				deviceCaps.VRAM += memoryProps.memoryHeaps[i].size / 1024; // KBs
		}

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
		VkPhysicalDeviceLimits limits = properties.limits;

		deviceCaps.Name = properties.deviceName;

		deviceCaps.MaxImageDimension2D = limits.maxImageDimension2D;
		deviceCaps.MaxImageDimensionCube = limits.maxImageDimensionCube;
		deviceCaps.MaxImageArrayLayers = limits.maxImageArrayLayers;
		deviceCaps.MaxSamplerLodBias = limits.maxSamplerLodBias;
		deviceCaps.MaxSamplerAnisotropy = limits.maxSamplerAnisotropy;

		deviceCaps.MaxFramebufferWidth = limits.maxFramebufferWidth;
		deviceCaps.MaxFramebufferHeight = limits.maxFramebufferHeight;
		deviceCaps.MaxFramebufferLayers = limits.maxFramebufferLayers;
		deviceCaps.MaxFramebufferColorAttachments = limits.maxColorAttachments;

		deviceCaps.MaxUniformBufferRange = limits.maxUniformBufferRange;
		deviceCaps.MaxStorageBufferRange = limits.maxStorageBufferRange;
		deviceCaps.MaxPushConstantRange = limits.maxPushConstantsSize;

		deviceCaps.MaxBoundDescriptorSets = limits.maxBoundDescriptorSets;
		deviceCaps.MaxDescriptorSetSamplers = limits.maxDescriptorSetSamplers;
		deviceCaps.MaxDescriptorSetUnifromBuffers = limits.maxDescriptorSetUniformBuffers;
		deviceCaps.MaxDescriptorSetStorageBuffers = limits.maxDescriptorSetStorageBuffers;
		deviceCaps.MaxDescriptorSetSampledImages = limits.maxDescriptorSetSampledImages;
		deviceCaps.MaxDescriptorSetStorageImages = limits.maxDescriptorSetStorageImages;
		deviceCaps.MaxDescriptorSetInputAttachments = limits.maxDescriptorSetInputAttachments;

		deviceCaps.MaxViewportDimensions[0] = limits.maxViewportDimensions[0];
		deviceCaps.MaxViewportDimensions[1] = limits.maxViewportDimensions[1];
		deviceCaps.MaxClipDistances = limits.maxClipDistances;
		deviceCaps.MaxCullDistances = limits.maxCullDistances;
		deviceCaps.LineWidthRange[0] = limits.lineWidthRange[0];
		deviceCaps.LineWidthRange[1] = limits.lineWidthRange[1];

		deviceCaps.MaxVertexInputAttributes = limits.maxVertexInputAttributes;
		deviceCaps.MaxVertexInputBindingStride = limits.maxVertexInputBindingStride;
		deviceCaps.MaxFragmentInputComponents = limits.maxFragmentInputComponents;
		deviceCaps.MaxFragmentOutputAttachments = limits.maxFragmentOutputAttachments;

		deviceCaps.MaxComputeWorkGroupSize[0] = limits.maxComputeWorkGroupSize[0];
		deviceCaps.MaxComputeWorkGroupSize[1] = limits.maxComputeWorkGroupSize[1];
		deviceCaps.MaxComputeWorkGroupSize[2] = limits.maxComputeWorkGroupSize[2];
		deviceCaps.MaxComputeSharedMemorySize = limits.maxComputeSharedMemorySize;
		deviceCaps.MaxComputeWorkGroupInvocations = limits.maxComputeWorkGroupInvocations;

		deviceCaps.TimestampComputeAndGraphics = limits.timestampComputeAndGraphics;
		deviceCaps.TimestampPeriod = limits.timestampPeriod;
	}

	void VulkanDevice::QueueSubmit(const VkSubmitInfo* submitInfo, VkFence fence)
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		VK_CHECK(vkQueueSubmit(m_Queue, 1, submitInfo, fence));
	}

	bool VulkanDevice::CheckEnabledExtensions(const std::vector<const char*>& requiredExtensions)
	{
		uint32 supportedExtensionCount = 0;
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, NULL, &supportedExtensionCount, nullptr);

		std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, NULL, &supportedExtensionCount, supportedExtensions.data());

		std::vector<const char*> missingExtensions;
		for (uint32 i = 0; i < requiredExtensions.size(); ++i)
		{
			bool find = false;
			for (uint32 j = 0; j < supportedExtensions.size(); ++j)
			{
				if (!strcmp(requiredExtensions[i], supportedExtensions[j].extensionName))
				{
					find = true;
					break;
				}
			}

			// (?) Make exception for debug marker extension 
			if (!find && strcmp(requiredExtensions[i], VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
			{
				missingExtensions.push_back(requiredExtensions[i]);
				break;
			}
		}

		String message = "Device supported extensions: \n\t";
		for (auto ext : supportedExtensions)
			message += fmt::format("'{}'\n\t", ext.extensionName);
		
		ATN_LOG_TRACE(Vulkan, message);

		message = "Device required extensions: \n\t";
		for (auto ext : requiredExtensions)
			message += fmt::format("'{}'\n\t", ext);
		
		ATN_LOG_INFO(Vulkan, message);

		if (!missingExtensions.empty())
		{
			ATN_LOG_FATAL(Vulkan, "Current Physical Device does not support required device extensions!");

			message = "Missing extensions: \n\t";
			for (auto ext : missingExtensions)
				message += fmt::format("'{}'\n\t", ext);

			ATN_LOG_ERROR(Vulkan, message);
			ensuref(false);
		}

		return missingExtensions.empty();
	}

	bool VulkanDevice::CheckSupportedFeatures()
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

		bool featuresSupported =
			deviceFeatures2.features.geometryShader &&
			deviceFeatures2.features.wideLines &&
			deviceFeatures2.features.pipelineStatisticsQuery &&
			deviceFeatures2.features.samplerAnisotropy &&
			deviceFeatures2.features.shaderStorageImageWriteWithoutFormat &&
			demoteFeatures.shaderDemoteToHelperInvocation &&
			hostQueryResetFeatures.hostQueryReset;

		if (!featuresSupported)
		{
			ATN_LOG_FATAL(Vulkan, "Physical device required features are not supported on this gpu (try to update drivers).");
		}

		return featuresSupported;
	}
}
