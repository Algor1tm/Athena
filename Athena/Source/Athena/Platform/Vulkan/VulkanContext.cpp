#include "VulkanContext.h"

#include "Athena/Core/Application.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanContextData VulkanContext::s_Data;

	namespace Utils
	{
#ifdef ATN_DEBUG
		static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
			uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
		{
			switch (flags)
			{
			case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
				ATN_CORE_INFO_TAG("Vulkan", "Debug report: \n{}\n", pMessage); break;

			case VK_DEBUG_REPORT_WARNING_BIT_EXT:
				ATN_CORE_WARN_TAG("Vulkan", "Debug report: \n{}\n", pMessage); break;

			case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
				ATN_CORE_WARN_TAG("Vulkan", "'PERFORMANCE WARNING' Debug report: \n{}\n", pMessage); break;

			case VK_DEBUG_REPORT_ERROR_BIT_EXT:
				ATN_CORE_ERROR_TAG("Vulkan", "Debug report: \n{}\n", pMessage); break;
			}

			return VK_FALSE;
		}
#endif

		static bool CheckEnabledExtensions(const std::vector<const char*>& requiredExtensions)
		{
			uint32 supportedExtensionCount = 0;
			vkEnumerateInstanceExtensionProperties(NULL, &supportedExtensionCount, nullptr);

			std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
			vkEnumerateInstanceExtensionProperties(NULL, &supportedExtensionCount, supportedExtensions.data());

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

			String message = "Vulkan supported extensions: \n\t";
			for (auto ext : supportedExtensions)
				message += std::format("'{}'\n\t", ext.extensionName);

			ATN_CORE_TRACE_TAG("Vulkan", message);

			message = "Vulkan required extensions: \n\t";
			for (auto ext : requiredExtensions)
				message += std::format("'{}'\n\t", ext);

			ATN_CORE_INFO_TAG("Vulkan", message);

			if (!exists)
			{
				ATN_CORE_FATAL_TAG("Vulkan", "Current Vulkan API version does not support required extensions!");
				ATN_CORE_ASSERT(false);
			}

			return exists;
		}

		static bool CheckEnabledLayers(const std::vector<const char*>& requiredLayers)
		{
			uint32 supportedLayerCount = 0;
			vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);

			std::vector<VkLayerProperties> supportedLayers(supportedLayerCount);
			vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.data());

			uint32 findCount = 0;
			for (uint32 i = 0; i < supportedLayers.size(); ++i)
			{
				String layerName = supportedLayers[i].layerName;
				bool find = (std::find_if(requiredLayers.begin(), requiredLayers.end(), [&layerName](const char* name)
					{ return name == layerName; }) != requiredLayers.end());

				if (find)
					findCount++;
			}

			bool exists = findCount == requiredLayers.size();

			String message = "Vulkan supported layers: \n\t";
			for (auto layer : supportedLayers)
				message += std::format("'{}'\n\t", layer.layerName);

			ATN_CORE_TRACE_TAG("Vulkan", message);

			message = "Vulkan required layers: \n\t";
			for (auto layer : requiredLayers)
				message += std::format("'{}'\n\t", layer);

			ATN_CORE_INFO_TAG("Vulkan", message);

			if (!exists)
			{
				ATN_CORE_FATAL_TAG("Vulkan", "Current Vulkan API version does not support required layers!");
				ATN_CORE_ASSERT(false);
			}

			return true;
		}
	}


	void VulkanContext::Init()
	{
		// Create Vulkan Instance
		{
			// Select Vulkan Version
			uint32 version = 0;
			VK_CHECK(vkEnumerateInstanceVersion(&version));

			uint32 variant = VK_API_VERSION_VARIANT(version);
			uint32 major = VK_API_VERSION_MAJOR(version);
			uint32 minor = VK_API_VERSION_MINOR(version);
			uint32 patch = VK_API_VERSION_PATCH(version);

			ATN_CORE_INFO_TAG("Vulkan", "Version: {}.{}.{}.{}", variant, major, minor, patch);

			VkApplicationInfo appInfo = {};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pNext = nullptr;
			appInfo.pApplicationName = Application::Get().GetConfig().Name.c_str();
			appInfo.pEngineName = "Athena";
			appInfo.apiVersion = version;

			// Select Extensions
			// Note: Vulkan initializes before GLFW, cant call glfwGetRequiredInstanceExtensions
			std::vector<const char*> extensions = {
				"VK_KHR_surface",
				"VK_KHR_win32_surface",
				"VK_KHR_get_physical_device_properties2"
				};
		
			std::vector<const char*> layers;

#ifdef ATN_DEBUG
			extensions.push_back("VK_EXT_debug_report");
			layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
			
			Utils::CheckEnabledExtensions(extensions);
			Utils::CheckEnabledLayers(layers);

			// Create Vulkan Instance
			VkInstanceCreateInfo instanceCI = {};
			instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instanceCI.pNext = nullptr;
			instanceCI.flags = 0;
			instanceCI.pApplicationInfo = &appInfo;
			instanceCI.enabledLayerCount = layers.size();
			instanceCI.ppEnabledLayerNames = layers.data();
			instanceCI.enabledExtensionCount = extensions.size();
			instanceCI.ppEnabledExtensionNames = extensions.data();

			VK_CHECK(vkCreateInstance(&instanceCI, nullptr, &s_Data.Instance));

#ifdef ATN_DEBUG
			// Setup the debug report callback
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(VulkanContext::GetInstance(), "vkCreateDebugReportCallbackEXT");
			ATN_CORE_ASSERT(vkCreateDebugReportCallbackEXT != NULL);

			VkDebugReportCallbackCreateInfoEXT reportCI = {};
			reportCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			reportCI.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			reportCI.pfnCallback = Utils::VulkanDebugCallback;
			reportCI.pUserData = NULL;
			VK_CHECK(vkCreateDebugReportCallbackEXT(VulkanContext::GetInstance(), &reportCI, nullptr, &s_Data.DebugReport));
#endif
		}

		// Create Device
		{
			s_Data.CurrentDevice = Ref<VulkanDevice>::Create();
		}

		// Create Allocator
		{
			uint32 version = 0;
			VK_CHECK(vkEnumerateInstanceVersion(&version));

			s_Data.Allocator = Ref<VulkanAllocator>::Create(version);
		}

		// Create synchronization primitives
		{
			s_Data.FrameSyncData.resize(Renderer::GetFramesInFlight());

			for (uint32_t i = 0; i < Renderer::GetFramesInFlight(); i++)
			{
				VkSemaphoreCreateInfo semaphoreCI = {};
				semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				VK_CHECK(vkCreateSemaphore(VulkanContext::GetLogicalDevice(), &semaphoreCI, nullptr, &s_Data.FrameSyncData[i].ImageAcquiredSemaphore));
				VK_CHECK(vkCreateSemaphore(VulkanContext::GetLogicalDevice(), &semaphoreCI, nullptr, &s_Data.FrameSyncData[i].RenderCompleteSemaphore));

				VkFenceCreateInfo fenceCI = {};
				fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				VK_CHECK(vkCreateFence(VulkanContext::GetLogicalDevice(), &fenceCI, nullptr, &s_Data.FrameSyncData[i].RenderCompleteFence));

			}
		}

		// Create CommandPool
		{
			VkCommandPoolCreateInfo commandPoolCI = {};
			commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCI.queueFamilyIndex = VulkanContext::GetDevice()->GetQueueFamily();
			commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			VK_CHECK(vkCreateCommandPool(VulkanContext::GetLogicalDevice(), &commandPoolCI, nullptr, &s_Data.CommandPool));
		}

		// Create Descriptors pool
		{
			VkDescriptorPoolSize poolSize = {};
			poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSize.descriptorCount = Renderer::GetFramesInFlight();

			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = 1;
			poolInfo.pPoolSizes = &poolSize;
			poolInfo.maxSets = Renderer::GetFramesInFlight();

			VK_CHECK(vkCreateDescriptorPool(VulkanContext::GetLogicalDevice(), &poolInfo, nullptr, &s_Data.DescriptorPool));
		}
	}

	void VulkanContext::Shutdown()
	{
		vkDestroyDescriptorPool(VulkanContext::GetLogicalDevice(), s_Data.DescriptorPool, nullptr);
		vkDestroyCommandPool(VulkanContext::GetLogicalDevice(), s_Data.CommandPool, nullptr);

		for (uint32_t i = 0; i < Renderer::GetFramesInFlight(); i++)
		{
			vkDestroySemaphore(VulkanContext::GetLogicalDevice(), s_Data.FrameSyncData[i].ImageAcquiredSemaphore, nullptr);
			vkDestroySemaphore(VulkanContext::GetLogicalDevice(), s_Data.FrameSyncData[i].RenderCompleteSemaphore, nullptr);

			vkDestroyFence(VulkanContext::GetLogicalDevice(), s_Data.FrameSyncData[i].RenderCompleteFence, nullptr);
		}

#ifdef ATN_DEBUG
		auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(s_Data.Instance, "vkDestroyDebugReportCallbackEXT");
		vkDestroyDebugReportCallbackEXT(VulkanContext::GetInstance(), s_Data.DebugReport, nullptr);
#endif

		s_Data.Allocator.Release();
		s_Data.CurrentDevice.Release();
		
		vkDestroyInstance(VulkanContext::GetInstance(), nullptr);
	}
}
