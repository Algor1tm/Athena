#include "VulkanContext.h"

#include "Athena/Core/Application.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanContextData VulkanContext::s_Data;

	namespace Utils
	{
		static bool CheckMinSupportedVersion(uint32 variant, uint32 major, uint32 minor, uint32 patch)
		{
			const uint32 minVariant = VK_API_VERSION_VARIANT(VULKAN_MIN_SUPPORTED_VERSION);
			const uint32 minMajor = VK_API_VERSION_MAJOR(VULKAN_MIN_SUPPORTED_VERSION);
			const uint32 minMinor = VK_API_VERSION_MINOR(VULKAN_MIN_SUPPORTED_VERSION);
			const uint32 minPatch = VK_API_VERSION_PATCH(VULKAN_MIN_SUPPORTED_VERSION);

			ATN_CORE_INFO_TAG("Vulkan", "Min supported version: {}.{}.{}.{}", minVariant, minMajor, minMinor, minPatch);

			if (variant > minVariant)
				return true;
			else if (variant < minVariant)
				return false;

			if (major > minMajor)
				return true;
			else if (major < minMajor)
				return false;

			if (minor > minMinor)
				return true;
			else if (minor < minMinor)
				return false;

			if (patch > minPatch)
				return true;
			else if (patch < minPatch)
				return false;

			return true;
		}

		static bool CheckEnabledExtensions(const std::vector<const char*>& requiredExtensions)
		{
			uint32 supportedExtensionCount = 0;
			vkEnumerateInstanceExtensionProperties(NULL, &supportedExtensionCount, nullptr);

			std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
			vkEnumerateInstanceExtensionProperties(NULL, &supportedExtensionCount, supportedExtensions.data());

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

				if (!find)
				{
					missingExtensions.push_back(requiredExtensions[i]);
					break;
				}
			}

			String message = "Vulkan supported extensions: \n\t";
			for (auto ext : supportedExtensions)
				message += std::format("'{}'\n\t", ext.extensionName);

			ATN_CORE_TRACE_TAG("Vulkan", message);

			message = "Vulkan required extensions: \n\t";
			for (auto ext : requiredExtensions)
				message += std::format("'{}'\n\t", ext);

			ATN_CORE_INFO_TAG("Vulkan", message);

			if (!missingExtensions.empty())
			{
				ATN_CORE_FATAL_TAG("Vulkan", "Current Vulkan version does not support required instance extensions!");

				message = "Missing extensions: \n\t";
				for (auto ext : missingExtensions)
					message += std::format("'{}'\n\t", ext);

				ATN_CORE_ERROR_TAG("Vulkan", message);
				ATN_CORE_VERIFY(false);
			}

			return missingExtensions.empty();
		}

		static bool CheckEnabledLayers(const std::vector<const char*>& requiredLayers)
		{
			uint32 supportedLayerCount = 0;
			vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);

			std::vector<VkLayerProperties> supportedLayers(supportedLayerCount);
			vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.data());

			std::vector<const char*> missingLayers;
			for (uint32 i = 0; i < requiredLayers.size(); ++i)
			{
				bool find = false;
				for (uint32 j = 0; j < supportedLayers.size(); ++j)
				{
					if (!strcmp(requiredLayers[i], supportedLayers[j].layerName))
					{
						find = true;
						break;
					}
				}

				if (!find)
				{
					missingLayers.push_back(requiredLayers[i]);
					break;
				}
			}
			String message = "Vulkan supported layers: \n\t";
			for (auto layer : supportedLayers)
				message += std::format("'{}'\n\t", layer.layerName);

			ATN_CORE_TRACE_TAG("Vulkan", message);

			message = "Vulkan required layers: \n\t";
			for (auto layer : requiredLayers)
				message += std::format("'{}'\n\t", layer);

			ATN_CORE_INFO_TAG("Vulkan", message);

			if (!missingLayers.empty())
			{
				ATN_CORE_FATAL_TAG("Vulkan", "Current Vulkan version does not support required instance layers!");

				message = "Missing layers: \n\t";
				for (auto layer : missingLayers)
					message += std::format("'{}'\n\t", layer);

				ATN_CORE_ERROR_TAG("Vulkan", message);
				ATN_CORE_VERIFY(false);
			}

			return missingLayers.empty();
		}
	}


	void VulkanContext::Init()
	{
		// Create Vulkan Instance
		{
			// Select Vulkan Version
			uint32 currentVersion = 0;
			VK_CHECK(vkEnumerateInstanceVersion(&currentVersion));

			uint32 variant = VK_API_VERSION_VARIANT(currentVersion);
			uint32 major = VK_API_VERSION_MAJOR(currentVersion);
			uint32 minor = VK_API_VERSION_MINOR(currentVersion);
			uint32 patch = VK_API_VERSION_PATCH(currentVersion);

			ATN_CORE_INFO_TAG("Vulkan", "Version: {}.{}.{}.{}", variant, major, minor, patch);

			if (!Utils::CheckMinSupportedVersion(variant, major, minor, patch))
			{
				ATN_CORE_FATAL_TAG("Vulkan", "Current Vulkan version is unsupported!");
				ATN_CORE_VERIFY(false);
			}

			VkApplicationInfo appInfo = {};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pNext = nullptr;
			appInfo.pApplicationName = Application::Get().GetConfig().Name.c_str();
			appInfo.pEngineName = "Athena";
#ifdef ATN_DEBUG
			appInfo.apiVersion = VULKAN_MIN_SUPPORTED_VERSION;
#else
			appInfo.apiVersion = currentVersion;
#endif
			// Select Extensions
			// NOTE: Vulkan initializes before GLFW, cant call glfwGetRequiredInstanceExtensions
			std::vector<const char*> extensions = {
				"VK_KHR_surface",
				"VK_KHR_win32_surface",
				"VK_KHR_get_physical_device_properties2"
				};
		
			std::vector<const char*> layers;

#ifdef VULKAN_ENABLE_DEBUG_INFO
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

#ifdef VULKAN_ENABLE_DEBUG_INFO
			// Setup the debug report callback
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(VulkanContext::GetInstance(), "vkCreateDebugReportCallbackEXT");
			ATN_CORE_ASSERT(vkCreateDebugReportCallbackEXT != NULL);

			VkDebugReportCallbackCreateInfoEXT reportCI = {};
			reportCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			reportCI.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT; // | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			reportCI.pfnCallback = Vulkan::DebugCallback;
			reportCI.pUserData = NULL;
			VK_CHECK(vkCreateDebugReportCallbackEXT(VulkanContext::GetInstance(), &reportCI, nullptr, &s_Data.DebugReport));
#endif
		}

		// Create Device
		{
			s_Data.Device = Ref<VulkanDevice>::Create();
		}

		// Create Allocator
		{
			uint32 version = 0;
			VK_CHECK(vkEnumerateInstanceVersion(&version));

			s_Data.Allocator = Ref<VulkanAllocator>::Create(version);
			s_Data.DescriptorSetAllocator = Ref<DescriptorSetAllocator>::Create();
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
	}

	void VulkanContext::Shutdown()
	{
		vkDestroyCommandPool(VulkanContext::GetLogicalDevice(), s_Data.CommandPool, nullptr);

		for (uint32_t i = 0; i < Renderer::GetFramesInFlight(); i++)
		{
			vkDestroySemaphore(VulkanContext::GetLogicalDevice(), s_Data.FrameSyncData[i].ImageAcquiredSemaphore, nullptr);
			vkDestroySemaphore(VulkanContext::GetLogicalDevice(), s_Data.FrameSyncData[i].RenderCompleteSemaphore, nullptr);

			vkDestroyFence(VulkanContext::GetLogicalDevice(), s_Data.FrameSyncData[i].RenderCompleteFence, nullptr);
		}

#ifdef VULKAN_ENABLE_DEBUG_INFO
		auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(s_Data.Instance, "vkDestroyDebugReportCallbackEXT");
		vkDestroyDebugReportCallbackEXT(VulkanContext::GetInstance(), s_Data.DebugReport, nullptr);
#endif

		s_Data.Allocator.Release();
		s_Data.DescriptorSetAllocator.Release();
		s_Data.Device.Release();
		
		vkDestroyInstance(VulkanContext::GetInstance(), nullptr);
	}
}
