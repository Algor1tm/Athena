#include "VulkanRenderer.h"

#include "Athena/Core/Application.h"
#include "Athena/Platform/Vulkan/VulkanDevice.h"
#include "Athena/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"

#include <GLFW/glfw3.h>


namespace Athena
{
	VkInstance VulkanContext::s_Instance = VK_NULL_HANDLE;
	VkAllocationCallbacks* VulkanContext::s_Allocator = VK_NULL_HANDLE;
	Ref<VulkanDevice> VulkanContext::s_CurrentDevice = VK_NULL_HANDLE;
	std::vector<FrameSyncData> VulkanContext::s_FrameSyncData;
	VkCommandPool VulkanContext::s_CommandPool = VK_NULL_HANDLE;
	VkCommandBuffer VulkanContext::s_ActiveCommandBuffer = VK_NULL_HANDLE;

#ifdef ATN_DEBUG
	inline VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
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

	void VulkanRenderer::Init()
	{
		ATN_CORE_VERIFY(VulkanContext::s_Instance == VK_NULL_HANDLE, "Vulkan Instance already exists!");

		// TODO: Add custom allocator
		VulkanContext::s_Allocator = VK_NULL_HANDLE;

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
			appInfo.pApplicationName = Application::Get().GetName().c_str();
			appInfo.pEngineName = "Athena";
			appInfo.apiVersion = version;

			// Select Extensions
			// Note: Vulkan initializes before GLFW, cant call glfwGetRequiredInstanceExtensions
			// maybe try other ways to initalize application
			std::vector<const char*> extensions = { 
				"VK_KHR_surface", 
				"VK_KHR_win32_surface" };

			std::vector<const char*> layers;

#ifdef ATN_DEBUG
			layers.push_back("VK_LAYER_KHRONOS_validation");
			extensions.push_back("VK_EXT_debug_report");
#endif

			String message = "Vulkan Extensions: \n\t";
			for (auto ext : extensions)
				message.append(std::format("'{}'\n\t", ext));

			ATN_CORE_INFO_TAG("Vulkan", message);

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

			VK_CHECK(vkCreateInstance(&instanceCI, VulkanContext::s_Allocator, &VulkanContext::s_Instance));

#ifdef ATN_DEBUG
			// Setup the debug report callback
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(VulkanContext::GetInstance(), "vkCreateDebugReportCallbackEXT");
			ATN_CORE_ASSERT(vkCreateDebugReportCallbackEXT != NULL);

			VkDebugReportCallbackCreateInfoEXT reportCI = {};
			reportCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			reportCI.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			reportCI.pfnCallback = VulkanDebugCallback;
			reportCI.pUserData = NULL;
			VK_CHECK(vkCreateDebugReportCallbackEXT(VulkanContext::GetInstance(), &reportCI, VulkanContext::GetAllocator(), &m_DebugReport));
#endif
		}

		// Create Device
		{
			VulkanContext::s_CurrentDevice = Ref<VulkanDevice>::Create();
		}

		// Create synchronization primitives
		{
			VulkanContext::s_FrameSyncData.resize(Renderer::GetFramesInFlight());

			for (uint32_t i = 0; i < Renderer::GetFramesInFlight(); i++)
			{
				VkSemaphoreCreateInfo semaphoreCI = {};
				semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				// Semaphore used to ensure that image acquired from swapchain before starting to submit again
				VK_CHECK(vkCreateSemaphore(VulkanContext::GetLogicalDevice(), &semaphoreCI, VulkanContext::GetAllocator(), &VulkanContext::s_FrameSyncData[i].ImageAcquiredSemaphore));
				// Semaphore used to ensure that all commands submitted have been finished before submitting the image to the queue
				VK_CHECK(vkCreateSemaphore(VulkanContext::GetLogicalDevice(), &semaphoreCI, VulkanContext::GetAllocator(), &VulkanContext::s_FrameSyncData[i].RenderCompleteSemaphore));

				VkFenceCreateInfo fenceCI = {};
				fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				// Create in signaled state so we don't wait on first render of each command buffer
				fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				VK_CHECK(vkCreateFence(VulkanContext::GetLogicalDevice(), &fenceCI, VulkanContext::GetAllocator(), &VulkanContext::s_FrameSyncData[i].RenderCompleteFence));

			}
		}

		// Create CommandPool
		{
			VkCommandPoolCreateInfo commandPoolCI = {};
			commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCI.queueFamilyIndex = VulkanContext::GetDevice()->GetQueueFamily();
			commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			VK_CHECK(vkCreateCommandPool(VulkanContext::GetLogicalDevice(), &commandPoolCI, VulkanContext::GetAllocator(), &VulkanContext::s_CommandPool));
		}
	}

	void VulkanRenderer::Shutdown()
	{
		VkDevice logicalDevice = VulkanContext::GetLogicalDevice();

		vkDestroyCommandPool(logicalDevice, VulkanContext::GetCommandPool(), VulkanContext::GetAllocator());

		for (uint32_t i = 0; i < Renderer::GetFramesInFlight(); i++)
		{
			vkDestroySemaphore(logicalDevice, VulkanContext::GetFrameSyncData(i).ImageAcquiredSemaphore, VulkanContext::GetAllocator());
			vkDestroySemaphore(logicalDevice, VulkanContext::GetFrameSyncData(i).RenderCompleteSemaphore, VulkanContext::GetAllocator());

			vkDestroyFence(logicalDevice, VulkanContext::GetFrameSyncData(i).RenderCompleteFence, VulkanContext::GetAllocator());
		}

		// Destroy Device
		VulkanContext::s_CurrentDevice.Reset();

#ifdef ATN_DEBUG
		auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(VulkanContext::GetInstance(), "vkDestroyDebugReportCallbackEXT");
		vkDestroyDebugReportCallbackEXT(VulkanContext::GetInstance(), m_DebugReport, VulkanContext::GetAllocator());
#endif

		vkDestroyInstance(VulkanContext::GetInstance(), VulkanContext::GetAllocator());
	}

	void VulkanRenderer::WaitDeviceIdle()
	{
		vkDeviceWaitIdle(VulkanContext::GetDevice()->GetLogicalDevice());
	}
}
