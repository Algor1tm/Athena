#include "VulkanDevice.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanDevice::VulkanDevice()
	{
		// Select GPU
		Renderer::Submit([this]()
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
		});

		
		// Select graphics queue family
		Renderer::Submit([this]()
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
				if ((queues[i].queueFlags & requestedQueueFlags) && (m_QueueFamily == UINT32_MAX))
					m_QueueFamily = i;

				String flags;

				flags += queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ? "Graphics, " : "";
				flags += queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT ? "Compute, " : "";
				flags += queues[i].queueFlags & VK_QUEUE_TRANSFER_BIT ? "Transfer, " : "";
				flags += queues[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? "SparseBinding, " : "";

				message += std::format("{}: {} count = {}\n\t", i, flags, queues[i].queueCount);


			}

			ATN_CORE_INFO_TAG("Vulkan", message);
			ATN_CORE_VERIFY(m_QueueFamily != UINT32_MAX, "Failed to find queue family that supports VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT");
		});

		// Create Logical Device
		Renderer::Submit([this]()
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

			std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

			CheckEnabledExtensions(deviceExtensions);

			ATN_CORE_INFO_TAG("Vulkan", message);

			VkDeviceCreateInfo deviceCI = {};
			deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceCI.queueCreateInfoCount = std::size(queueCIs);
			deviceCI.pQueueCreateInfos = queueCIs;
			deviceCI.enabledExtensionCount = deviceExtensions.size();
			deviceCI.ppEnabledExtensionNames = deviceExtensions.data();

			VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCI, VulkanContext::GetAllocator(), &m_LogicalDevice));
			vkGetDeviceQueue(m_LogicalDevice, m_QueueFamily, 0, &m_Queue);
		});
	}

	VulkanDevice::~VulkanDevice()
	{
		vkDestroyDevice(m_LogicalDevice, VulkanContext::GetAllocator());
	}

	void VulkanDevice::WaitIdle()
	{
		vkDeviceWaitIdle(m_LogicalDevice);
	}

	RenderCapabilities VulkanDevice::GetDeviceCapabilities() const
	{
		RenderCapabilities deviceCaps;

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

		deviceCaps.Name = properties.deviceName;

		deviceCaps.VRAM = 0; // TODO

		deviceCaps.MaxImageDimension2D = properties.limits.maxImageDimension2D;
		deviceCaps.MaxImageDimensionCube = properties.limits.maxImageDimensionCube;
		deviceCaps.MaxImageArrayLayers = properties.limits.maxImageArrayLayers;
		deviceCaps.MaxSamplerLodBias = properties.limits.maxSamplerLodBias;
		deviceCaps.MaxSamplerAnisotropy = properties.limits.maxSamplerAnisotropy;

		deviceCaps.MaxFramebufferWidth = properties.limits.maxFramebufferWidth;
		deviceCaps.MaxFramebufferHeight = properties.limits.maxFramebufferHeight;
		deviceCaps.MaxFramebufferLayers = properties.limits.maxFramebufferLayers;
		deviceCaps.MaxFramebufferColorAttachments = properties.limits.maxColorAttachments;

		deviceCaps.MaxUniformBufferRange = properties.limits.maxUniformBufferRange;
		deviceCaps.MaxStorageBufferRange = properties.limits.maxStorageBufferRange;
		deviceCaps.MaxPushConstantRange = properties.limits.maxPushConstantsSize;

		deviceCaps.MaxViewportDimensions[0] = properties.limits.maxViewportDimensions[0];
		deviceCaps.MaxViewportDimensions[1] = properties.limits.maxViewportDimensions[1];
		deviceCaps.MaxClipDistances = properties.limits.maxClipDistances;
		deviceCaps.MaxCullDistances = properties.limits.maxCullDistances;
		deviceCaps.LineWidthRange[0] = properties.limits.lineWidthRange[0];
		deviceCaps.LineWidthRange[1] = properties.limits.lineWidthRange[1];

		deviceCaps.MaxVertexInputAttributes = properties.limits.maxVertexInputAttributes;
		deviceCaps.MaxVertexInputBindingStride = properties.limits.maxVertexInputBindingStride;
		deviceCaps.MaxFragmentInputComponents = properties.limits.maxFragmentInputComponents;
		deviceCaps.MaxFragmentOutputAttachments = properties.limits.maxFragmentOutputAttachments;

		deviceCaps.MaxComputeWorkGroupSize[0] = properties.limits.maxComputeWorkGroupSize[0];
		deviceCaps.MaxComputeWorkGroupSize[1] = properties.limits.maxComputeWorkGroupSize[1];
		deviceCaps.MaxComputeWorkGroupSize[2] = properties.limits.maxComputeWorkGroupSize[2];
		deviceCaps.MaxComputeSharedMemorySize = properties.limits.maxComputeSharedMemorySize;
		deviceCaps.MaxComputeWorkGroupInvocations = properties.limits.maxComputeWorkGroupInvocations;

		return deviceCaps;
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
