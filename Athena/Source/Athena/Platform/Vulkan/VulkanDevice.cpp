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

				message += std::format("{}\n\t", properties.deviceName);
			}

			ATN_CORE_INFO_TAG("Vulkan", message);
			ATN_CORE_INFO_TAG("Vulkan", "Selected GPU: {}\n", selectedGPUName);

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

				message += std::format("{}: {} timestamps = {}, count = {}\n\t", i, flags, queues[i].timestampValidBits > 0, queues[i].queueCount);
			}

			ATN_CORE_INFO_TAG("Vulkan", message);
			ATN_CORE_VERIFY(m_QueueFamily != UINT32_MAX, "Failed to find queue family that supports VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT operations and timestamps");
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

			message += std::format("QueueFamily - {}, count - {}\n\t", m_QueueFamily, 1);
			ATN_CORE_INFO_TAG("Vulkan", message);

			std::vector<const char*> deviceExtensions = { 
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				"VK_EXT_memory_budget" };

			CheckEnabledExtensions(deviceExtensions);

			// GPU profiling
			VkPhysicalDeviceHostQueryResetFeatures resetFeatures = {};
			resetFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;
			resetFeatures.pNext = nullptr;
			resetFeatures.hostQueryReset = VK_TRUE;

			VkPhysicalDeviceFeatures deviceFeatures = {};
			deviceFeatures.geometryShader = VK_TRUE;
			deviceFeatures.wideLines = VK_TRUE;
			deviceFeatures.pipelineStatisticsQuery = VK_TRUE;

			VkDeviceCreateInfo deviceCI = {};
			deviceCI.pNext = &resetFeatures;
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
				deviceCaps.VRAM += memoryProps.memoryHeaps[i].size / 1024; // bytes
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

	bool VulkanDevice::CheckEnabledExtensions(const std::vector<const char*>& requiredExtensions)
	{
		uint32 supportedExtensionCount = 0;
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, NULL, &supportedExtensionCount, nullptr);

		std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, NULL, &supportedExtensionCount, supportedExtensions.data());

		uint32 findCount = 0;
		for (uint32 i = 0; i < supportedExtensions.size(); ++i)
		{
			String extName = supportedExtensions[i].extensionName;
			bool find = (std::find_if(requiredExtensions.begin(), requiredExtensions.end(), [&extName](const char* name)
				{ return name == extName; }) != requiredExtensions.end());

			if (find)
				findCount++;
		}

		bool exists = findCount == requiredExtensions.size();

		String message = "Device supported extensions: \n\t";
		for (auto ext : supportedExtensions)
			message += std::format("'{}'\n\t", ext.extensionName);

		ATN_CORE_TRACE_TAG("Vulkan", message);

		message = "Device required extensions: \n\t";
		for (auto ext : requiredExtensions)
			message += std::format("'{}'\n\t", ext);

		ATN_CORE_INFO_TAG("Vulkan", message);

		if (!exists)
		{
			ATN_CORE_FATAL_TAG("Vulkan", "Current Physical Device does not support required extensions!");
			ATN_CORE_ASSERT(false);
		}

		return exists;
	}
}
