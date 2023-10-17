#include "VulkanDevice.h"

#include "Athena/Platform/Vulkan/VulkanContext.h"


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
		}

		// Select graphics queue family
		{
			uint32 count;
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, NULL);

			std::vector<VkQueueFamilyProperties> queues(count);
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, queues.data());

			String message = "Queue Families: \n\t";

			VkQueueFlagBits requestedFlags[] = { VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT };
			for (uint32 i = 0; i < count; i++)
			{
				bool supportFlags = true;
				for (auto flag : requestedFlags)
				{
					if (!(queues[i].queueFlags & flag))
					{
						supportFlags = false;
						break;
					}
				}

				if (supportFlags)
					m_QueueFamily = i;

				String flags;

				flags += queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ? "Graphics, " : "";
				flags += queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT ? "Compute, " : "";
				flags += queues[i].queueFlags & VK_QUEUE_TRANSFER_BIT ? "Transfer, " : "";
				flags += queues[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? "SparseBinding, " : "";

				message += std::format("{}: {} count = {}\n\t", i, flags, queues[i].queueCount);
			}

			ATN_CORE_INFO_TAG("Vulkan", message);
			ATN_CORE_VERIFY(m_QueueFamily != (uint32)-1);
		}

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

			std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

			message = "Device extensions: \n\t";
			for (auto ext : deviceExtensions)
				message += std::format("{}\n\t", ext);

			ATN_CORE_INFO_TAG("Vulkan", message);

			VkDeviceCreateInfo deviceCI = {};
			deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceCI.queueCreateInfoCount = std::size(queueCIs);
			deviceCI.pQueueCreateInfos = queueCIs;
			deviceCI.enabledExtensionCount = deviceExtensions.size();
			deviceCI.ppEnabledExtensionNames = deviceExtensions.data();

			VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCI, VulkanContext::GetAllocator(), &m_LogicalDevice));
			vkGetDeviceQueue(m_LogicalDevice, m_QueueFamily, 0, &m_Queue);
		}
	}

	VulkanDevice::~VulkanDevice()
	{
		vkDestroyDevice(m_LogicalDevice, VulkanContext::GetAllocator());
	}

	void VulkanDevice::WaitIdle()
	{
		vkDeviceWaitIdle(m_LogicalDevice);
	}
}
