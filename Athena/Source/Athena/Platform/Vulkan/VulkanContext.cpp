#include "VulkanContext.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/ConsoleManager.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"

namespace Athena
{
	static AutoCVar<int32> CVarVulkanRequestedMajorVersion(
		"Vulkan.RequestedMajorVersion",
		1,
		"Requests this version for major, set -1 to use latest."
	);

	static AutoCVar<int32> CVarVulkanRequestedMinorVersion(
		"Vulkan.RequestedMinorVersion",
		2,
		"Requests this version for minor, set -1 to use latest."
	);

	static AutoCVar<int32> CVarVulkanRequestedPatchVersion(
		"Vulkan.RequestedPatchVersion",
		-1,
		"Requests this version for patch, set -1 to use latest."
	);

	VulkanContextData VulkanContext::s_Data;

	namespace Utils
	{
		static uint32 QueryVulkanVersion()
		{
			uint32 latestVersion = 0;
			VK_CHECK(vkEnumerateInstanceVersion(&latestVersion));

			const uint32 supportedVariant = VK_API_VERSION_VARIANT(latestVersion);
			const uint32 supportedMajor = VK_API_VERSION_MAJOR(latestVersion);
			const uint32 supportedMinor = VK_API_VERSION_MINOR(latestVersion);
			const uint32 supportedPatch = VK_API_VERSION_PATCH(latestVersion);

			int32 requestedVariant = 0;
			int32 requestedMajor = CVarVulkanRequestedMajorVersion.GetInt();
			int32 requestedMinor = CVarVulkanRequestedMinorVersion.GetInt();
			int32 requestedPatch = CVarVulkanRequestedPatchVersion.GetInt();

			if (requestedVariant > supportedVariant || requestedVariant < 0)
				requestedVariant = supportedVariant;

			if (requestedMajor > supportedMajor || requestedMajor < 0)
				requestedMajor = supportedMajor;

			if (requestedMinor > supportedMinor || requestedMinor < 0)
				requestedMinor = supportedMinor;

			if (requestedPatch > supportedPatch || requestedPatch < 0)
				requestedPatch = supportedPatch;

			return VK_MAKE_API_VERSION(requestedVariant, requestedMajor, requestedMinor, requestedPatch);
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
				message += fmt::format("'{}'\n\t", ext.extensionName);

			ATN_LOG_TRACE(Vulkan, message);

			message = "Vulkan required extensions: \n\t";
			for (auto ext : requiredExtensions)
				message += fmt::format("'{}'\n\t", ext);

			ATN_LOG_INFO(Vulkan, message);

			if (!missingExtensions.empty())
			{
				ATN_LOG_FATAL(Vulkan, "Current Vulkan version does not support required instance extensions!");

				message = "Missing extensions: \n\t";
				for (auto ext : missingExtensions)
					message += fmt::format("'{}'\n\t", ext);

				ATN_LOG_ERROR(Vulkan, message);
				ensuref(false);
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
				message += fmt::format("'{}'\n\t", layer.layerName);

			ATN_LOG_TRACE(Vulkan, message);

			message = "Vulkan required layers: \n\t";
			for (auto layer : requiredLayers)
				message += fmt::format("'{}'\n\t", layer);

			ATN_LOG_INFO(Vulkan, message);

			if (!missingLayers.empty())
			{
				ATN_LOG_FATAL(Vulkan, "Current Vulkan version does not support required instance layers!");

				message = "Missing layers: \n\t";
				for (auto layer : missingLayers)
					message += fmt::format("'{}'\n\t", layer);

				ATN_LOG_ERROR(Vulkan, message);
				ensuref(false);
			}

			return missingLayers.empty();
		}
	}


	void VulkanContext::Init()
	{
		// Create Vulkan Instance
		{
			s_Data.InstanceVersion = Utils::QueryVulkanVersion();

			const uint32 variant = VK_API_VERSION_VARIANT(s_Data.InstanceVersion);
			const uint32 major = VK_API_VERSION_MAJOR(s_Data.InstanceVersion);
			const uint32 minor = VK_API_VERSION_MINOR(s_Data.InstanceVersion);
			const uint32 patch = VK_API_VERSION_PATCH(s_Data.InstanceVersion);

			ATN_LOG_INFO(Vulkan, "Selected Vulkan API version: {}.{}.{}.{}", variant, major, minor, patch);

			VkApplicationInfo appInfo = {};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pNext = nullptr;
			appInfo.pApplicationName = Application::Get().GetConfig().Name.c_str();
			appInfo.pEngineName = "Athena";
			appInfo.apiVersion = s_Data.InstanceVersion;

			// Select Extensions
			// NOTE: Vulkan initializes before GLFW, cant call glfwGetRequiredInstanceExtensions
			std::vector<const char*> extensions = {
				"VK_KHR_surface",
				"VK_KHR_win32_surface",
				"VK_KHR_get_physical_device_properties2"
				};
		
			std::vector<const char*> layers;

#if VULKAN_ENABLE_DEBUG_INFO
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

#if VULKAN_ENABLE_DEBUG_INFO
			// Setup the debug report callback
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(VulkanContext::GetInstance(), "vkCreateDebugReportCallbackEXT");
			checkf(vkCreateDebugReportCallbackEXT != NULL);

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
			s_Data.Allocator = Ref<VulkanAllocator>::Create(s_Data.InstanceVersion);
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

		s_Data.BindedPipelineLayout = VK_NULL_HANDLE;
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

#if VULKAN_ENABLE_DEBUG_INFO
		auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(s_Data.Instance, "vkDestroyDebugReportCallbackEXT");
		vkDestroyDebugReportCallbackEXT(VulkanContext::GetInstance(), s_Data.DebugReport, nullptr);
#endif

		s_Data.Allocator.Release();
		s_Data.DescriptorSetAllocator.Release();
		s_Data.Device.Release();
		
		vkDestroyInstance(VulkanContext::GetInstance(), nullptr);
	}
}
