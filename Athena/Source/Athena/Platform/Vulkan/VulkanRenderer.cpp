#include "VulkanRenderer.h"

#include "Athena/Core/Application.h"
#include "Athena/Platform/Vulkan/VulkanDebug.h"
#include "Athena/Platform/Vulkan/VulkanDevice.h"
#include "Athena/Platform/Vulkan/VulkanCommandBuffer.h"

#include <GLFW/glfw3.h>


namespace Athena
{
	VkInstance VulkanContext::s_Instance = VK_NULL_HANDLE;
	VkAllocationCallbacks* VulkanContext::s_Allocator = VK_NULL_HANDLE;
	Ref<VulkanDevice> VulkanContext::s_CurrentDevice = VK_NULL_HANDLE;
	std::vector<FrameSyncData> VulkanContext::s_FrameSyncData;
	VkCommandPool VulkanContext::s_CommandPool = VK_NULL_HANDLE;


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
			std::vector<const char*> extensions = { "VK_KHR_surface", "VK_KHR_win32_surface" }; //glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
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
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(VulkanContext::s_Instance, "vkCreateDebugReportCallbackEXT");
			ATN_CORE_ASSERT(vkCreateDebugReportCallbackEXT != NULL);

			VkDebugReportCallbackCreateInfoEXT reportCI = {};
			reportCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			reportCI.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			reportCI.pfnCallback = VulkanDebugCallback;
			reportCI.pUserData = NULL;
			VK_CHECK(vkCreateDebugReportCallbackEXT(VulkanContext::s_Instance, &reportCI, VulkanContext::s_Allocator, &m_DebugReport));
#endif
		}

		// Create Device
		{
			VulkanContext::s_CurrentDevice = Ref<VulkanDevice>::Create();
		}

		// Create synchronization primitives
		{
			VkDevice logicalDevice = VulkanContext::GetCurrentDevice()->GetLogicalDevice();

			VulkanContext::s_FrameSyncData.resize(Renderer::FramesInFlight());

			for (uint32_t i = 0; i < Renderer::FramesInFlight(); i++)
			{
				VkSemaphoreCreateInfo semaphoreCI = {};
				semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				// Semaphore used to ensure that image acquired from swapchain before starting to submit again
				VK_CHECK(vkCreateSemaphore(logicalDevice, &semaphoreCI, VulkanContext::s_Allocator, &VulkanContext::s_FrameSyncData[i].ImageAcquiredSemaphore));
				// Semaphore used to ensure that all commands submitted have been finished before submitting the image to the queue
				VK_CHECK(vkCreateSemaphore(logicalDevice, &semaphoreCI, VulkanContext::s_Allocator, &VulkanContext::s_FrameSyncData[i].RenderCompleteSemaphore));

				VkFenceCreateInfo fenceCI = {};
				fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				// Create in signaled state so we don't wait on first render of each command buffer
				fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				VK_CHECK(vkCreateFence(logicalDevice, &fenceCI, VulkanContext::s_Allocator, &VulkanContext::s_FrameSyncData[i].RenderCompleteFence));

			}
		}

		// Create CommandPool
		{
			Ref<VulkanDevice> device = VulkanContext::GetCurrentDevice();

			VkCommandPoolCreateInfo commandPoolCI = {};
			commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCI.queueFamilyIndex = device->GetQueueFamily();
			commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			VK_CHECK(vkCreateCommandPool(device->GetLogicalDevice(), &commandPoolCI, VulkanContext::s_Allocator, &VulkanContext::s_CommandPool));
		}
	}

	void VulkanRenderer::Shutdown()
	{
		
		VkDevice logicalDevice = VulkanContext::s_CurrentDevice->GetLogicalDevice();

		vkDestroyCommandPool(logicalDevice, VulkanContext::s_CommandPool, VulkanContext::s_Allocator);

		for (uint32_t i = 0; i < Renderer::FramesInFlight(); i++)
		{
			vkDestroySemaphore(logicalDevice, VulkanContext::s_FrameSyncData[i].ImageAcquiredSemaphore, VulkanContext::s_Allocator);
			vkDestroySemaphore(logicalDevice, VulkanContext::s_FrameSyncData[i].RenderCompleteSemaphore, VulkanContext::s_Allocator);

			vkDestroyFence(logicalDevice, VulkanContext::s_FrameSyncData[i].RenderCompleteFence, VulkanContext::s_Allocator);
		}

		// Destroy Device
		VulkanContext::s_CurrentDevice.Reset();

#ifdef ATN_DEBUG
		auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(VulkanContext::s_Instance, "vkDestroyDebugReportCallbackEXT");
		vkDestroyDebugReportCallbackEXT(VulkanContext::s_Instance, m_DebugReport, VulkanContext::s_Allocator);
#endif

		vkDestroyInstance(VulkanContext::s_Instance, VulkanContext::s_Allocator);
	}

	void VulkanRenderer::Submit(const Ref<CommandBuffer>& cmdBuff)
	{
		const FrameSyncData& frameData = VulkanContext::s_FrameSyncData[Renderer::CurrentFrameIndex()];

		VkCommandBuffer buffer = cmdBuff.As<VulkanCommandBuffer>()->GetNativeCmdBuffer();
		VkPipelineStageFlags waitStage[] = { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		// Submit commands to queue
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &frameData.ImageAcquiredSemaphore;
		submitInfo.pWaitDstStageMask = waitStage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &buffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &frameData.RenderCompleteSemaphore;

		VK_CHECK(vkQueueSubmit(VulkanContext::GetCurrentDevice()->GetQueue(), 1, &submitInfo, frameData.RenderCompleteFence));
	}

	void VulkanRenderer::WaitDeviceIdle()
	{
		vkDeviceWaitIdle(VulkanContext::GetCurrentDevice()->GetLogicalDevice());
	}
}
