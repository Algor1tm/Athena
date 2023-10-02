#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Window.h"

#include "Athena/Input/Input.h"
#include "Athena/Input/WindowEvent.h"
#include "Athena/Input/KeyEvent.h"
#include "Athena/Input/MouseEvent.h"

#include  "Athena/Platform/OpenGL/GLGraphicsContext.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <stb_image/stb_image.h>


namespace Athena
{
	static VkAllocationCallbacks*	s_Allocator = NULL;
	static VkInstance				s_Instance = VK_NULL_HANDLE;
	static VkPhysicalDevice         s_PhysicalDevice = VK_NULL_HANDLE;
	static VkDevice                 s_Device = VK_NULL_HANDLE;
	static uint32_t                 s_QueueFamily = (uint32_t)-1;
	static VkQueue                  s_Queue = VK_NULL_HANDLE;
	static VkSurfaceKHR				s_Surface = VK_NULL_HANDLE;
	static VkSwapchainKHR			s_SwapChain = VK_NULL_HANDLE;

	static VkCommandPool			s_CommandPool = VK_NULL_HANDLE;

	static VkDebugReportCallbackEXT s_DebugReport = VK_NULL_HANDLE;

	// Resources per frame in flight
	static std::vector<VkImage>		s_SwapChainImages;
	static std::vector<VkImageView>	s_SwapChainImageViews;

	static std::vector<VkCommandBuffer> s_CommandBuffers;

	static std::vector<VkSemaphore> s_ImageAcquiredSemaphores;
	static std::vector<VkSemaphore> s_RenderCompleteSemaphores;
	static std::vector<VkFence>		s_RenderCompleteFences;


	static uint32					s_CurrentFrameIndex = 0;
	static const uint32				s_MaxFramesInFlight = 2;


	static void check_vk_result(VkResult error)
	{
		if (error == 0)
			return;

		if (error > 0)
			ATN_CORE_ERROR_TAG("Vulkan", "Error: VkResult = {}", (int)error);

		if(error < 0)
			ATN_CORE_FATAL_TAG("Vulkan", "Fatal Error: VkResult = {}", (int)error);

		ATN_CORE_ASSERT(false);
	}

#define VK_CHECK(expr) check_vk_result(expr)

#ifdef ATN_DEBUG
	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, 
		uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
	{
		(void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
		ATN_CORE_ERROR_TAG("Vulkan", "Debug report from ObjectType: {} \nMessage: {}\n", (int)objectType, pMessage);
		return VK_FALSE;
	}
#endif // IMGUI_VULKAN_DEBUG_REPORT

	static void InitVulkan(GLFWwindow* window)
	{
		// Create Instance
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
			appInfo.pApplicationName = "Vulkan Example";
			appInfo.pEngineName = "Athena";
			appInfo.apiVersion = version;

			// Select Extensions
			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

			std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
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

			VK_CHECK(vkCreateInstance(&instanceCI, s_Allocator, &s_Instance));

#ifdef ATN_DEBUG
			// Setup the debug report callback
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(s_Instance, "vkCreateDebugReportCallbackEXT");
			ATN_CORE_ASSERT(vkCreateDebugReportCallbackEXT != NULL);

			VkDebugReportCallbackCreateInfoEXT reportCI = {};
			reportCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			reportCI.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			reportCI.pfnCallback = VulkanDebugCallback;
			reportCI.pUserData = NULL;
			VK_CHECK(vkCreateDebugReportCallbackEXT(s_Instance, &reportCI, s_Allocator, &s_DebugReport));
#endif
		}

		// Select GPU
		{
			uint32 gpuCount;
			VK_CHECK(vkEnumeratePhysicalDevices(s_Instance, &gpuCount, NULL));

			std::vector<VkPhysicalDevice> gpus(gpuCount);

			VK_CHECK(vkEnumeratePhysicalDevices(s_Instance, &gpuCount, gpus.data()));

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

			s_PhysicalDevice = gpus[useGpu];
		}

		// Select graphics queue family
		{
			uint32 count;
			vkGetPhysicalDeviceQueueFamilyProperties(s_PhysicalDevice, &count, NULL);

			std::vector<VkQueueFamilyProperties> queues(count);
			vkGetPhysicalDeviceQueueFamilyProperties(s_PhysicalDevice, &count, queues.data());

			String message = "Queue Families: \n\t";

			for (uint32 i = 0; i < count; i++)
			{
				if ((queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
				{
					s_QueueFamily = i;
				}

				String flags;

				flags += queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ? "Graphics, " : "";
				flags += queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT ? "Compute, " : "";
				flags += queues[i].queueFlags & VK_QUEUE_TRANSFER_BIT ? "Transfer, " : "";
				flags += queues[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? "SparseBinding, " : "";

				message += std::format("{}: {} count = {}\n\t", i, flags, queues[i].queueCount);
			}
			
			ATN_CORE_INFO_TAG("Vulkan", message);
			ATN_CORE_VERIFY(s_QueueFamily != (uint32)-1);
		}

		// Create Logical Device
		{
			String message = "Queues created: \n\t";

			const float queuePriority[] = { 1.0f };

			VkDeviceQueueCreateInfo queueCIs[1] = {};
			queueCIs[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCIs[0].queueFamilyIndex = s_QueueFamily;
			queueCIs[0].queueCount = 1;
			queueCIs[0].pQueuePriorities = queuePriority;

			message += std::format("QueueFamily - {}, count - {}\n\t", s_QueueFamily, 1);
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

			VK_CHECK(vkCreateDevice(s_PhysicalDevice, &deviceCI, s_Allocator, &s_Device));
			vkGetDeviceQueue(s_Device, s_QueueFamily, 0, &s_Queue);
		}

		// Create window surface and SwapChain
		{
			VK_CHECK(glfwCreateWindowSurface(s_Instance, window, s_Allocator, &s_Surface));

			// Check for WSI support
			VkBool32 res;
			vkGetPhysicalDeviceSurfaceSupportKHR(s_PhysicalDevice, s_QueueFamily, s_Surface, &res);
			ATN_CORE_VERIFY(res, "no WSI support on physical device");

			ATN_CORE_INFO_TAG("Vulkan", "Create Window Surface");

			VkSurfaceCapabilitiesKHR surfaceCaps;
			std::vector<VkSurfaceFormatKHR> surfaceFormats;
			std::vector<VkPresentModeKHR> surfacePresentModes;

			// Surface Capabilites
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s_PhysicalDevice, s_Surface, &surfaceCaps);

			ATN_CORE_VERIFY(surfaceCaps.minImageCount <= s_MaxFramesInFlight && surfaceCaps.maxImageCount >= s_MaxFramesInFlight);

			// Supported formats
			uint32 formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(s_PhysicalDevice, s_Surface, &formatCount, nullptr);

			surfaceFormats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(s_PhysicalDevice, s_Surface, &formatCount, surfaceFormats.data());

			// Supported present modes
			uint32 presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(s_PhysicalDevice, s_Surface, &presentModeCount, nullptr);

			surfacePresentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(s_PhysicalDevice, s_Surface, &presentModeCount, surfacePresentModes.data());
			
			// Select Format
			const VkFormat requestedFormats[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
			const VkColorSpaceKHR requestedColorsSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

			// first format in the array is prefered
			const auto rateFormat = [requestedFormats, requestedColorsSpace](VkSurfaceFormatKHR fmt) -> uint64
			{
				if (fmt.colorSpace != requestedColorsSpace)
					return 0;

				auto findPtr = std::find(std::begin(requestedFormats), std::end(requestedFormats), fmt.format);
				uint64 reversedIdx = std::distance(findPtr, std::end(requestedFormats)); // if last element index is 1

				return reversedIdx;
			};

			std::sort(surfaceFormats.begin(), surfaceFormats.end(), [rateFormat, requestedFormats, requestedColorsSpace](VkSurfaceFormatKHR left, VkSurfaceFormatKHR right)
				{
					return rateFormat(left) > rateFormat(right);
				});

			VkSurfaceFormatKHR selectedFormat = surfaceFormats[0];

			// Select Present mode
			int32 usePresentMode = -1;
			for (uint64 i = 0; i < surfacePresentModes.size(); ++i)
			{
				if (surfacePresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					usePresentMode = i;
					break;
				}
			}

			VkPresentModeKHR selectedPresentMode = usePresentMode == -1 ? VK_PRESENT_MODE_FIFO_KHR : surfacePresentModes[usePresentMode];


			// Extent
			Window::WindowData& windowData = *(Window::WindowData*)(glfwGetWindowUserPointer(window));

			VkExtent2D extent;
			if (surfaceCaps.currentExtent.width == (uint32)-1)
			{
				extent.width = windowData.Width;
				extent.height = windowData.Height;

				extent.width = Math::Clamp(extent.width, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width);
				extent.height = Math::Clamp(extent.width, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height);
			}
			else
			{
				extent.width = surfaceCaps.currentExtent.width;
				extent.height = surfaceCaps.currentExtent.height;
			}


			VkSwapchainCreateInfoKHR swapchainCI = {};
			swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapchainCI.surface = s_Surface;
			swapchainCI.minImageCount = s_MaxFramesInFlight;
			swapchainCI.imageFormat = selectedFormat.format;
			swapchainCI.imageColorSpace = selectedFormat.colorSpace;
			swapchainCI.imageExtent.width = extent.width;
			swapchainCI.imageExtent.height = extent.height;
			swapchainCI.imageArrayLayers = 1;
			swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;           // Assume that graphics family == present family
			swapchainCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapchainCI.presentMode = selectedPresentMode;
			swapchainCI.clipped = VK_TRUE;
			swapchainCI.oldSwapchain = s_SwapChain;

			VK_CHECK(vkCreateSwapchainKHR(s_Device, &swapchainCI, s_Allocator, &s_SwapChain));

			uint32 imagesCount;
			vkGetSwapchainImagesKHR(s_Device, s_SwapChain, &imagesCount, nullptr);
			ATN_CORE_VERIFY(imagesCount == s_MaxFramesInFlight);

			s_SwapChainImages.resize(s_MaxFramesInFlight);
			VK_CHECK(vkGetSwapchainImagesKHR(s_Device, s_SwapChain, &imagesCount, s_SwapChainImages.data()));

			s_SwapChainImageViews.resize(s_MaxFramesInFlight);
			for (uint32_t i = 0; i < s_MaxFramesInFlight; i++)
			{
				VkImageViewCreateInfo imageViewCI = {};
				imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				imageViewCI.format = selectedFormat.format;
				imageViewCI.components.r = VK_COMPONENT_SWIZZLE_R;
				imageViewCI.components.g = VK_COMPONENT_SWIZZLE_G;
				imageViewCI.components.b = VK_COMPONENT_SWIZZLE_B;
				imageViewCI.components.a = VK_COMPONENT_SWIZZLE_A;
				imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageViewCI.subresourceRange.baseMipLevel = 0;
				imageViewCI.subresourceRange.levelCount = 1;
				imageViewCI.subresourceRange.baseArrayLayer = 0;
				imageViewCI.subresourceRange.layerCount = 1;
				imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
				imageViewCI.image = s_SwapChainImages[i];

				VK_CHECK(vkCreateImageView(s_Device, &imageViewCI, s_Allocator, &s_SwapChainImageViews[i]));
			}
		}

		// Create CommandPool and CommandBuffers
		{
			VkCommandPoolCreateInfo commandPoolCI = {};
			commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCI.queueFamilyIndex = s_QueueFamily;
			commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			VK_CHECK(vkCreateCommandPool(s_Device, &commandPoolCI, nullptr, &s_CommandPool));

			s_CommandBuffers.resize(s_MaxFramesInFlight);

			VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
			cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufAllocInfo.commandPool = s_CommandPool;
			cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdBufAllocInfo.commandBufferCount = s_MaxFramesInFlight;
			VK_CHECK(vkAllocateCommandBuffers(s_Device, &cmdBufAllocInfo, s_CommandBuffers.data()));
		}
		
		// Create synchronization primitives
		{
			s_ImageAcquiredSemaphores.resize(s_MaxFramesInFlight);
			s_RenderCompleteSemaphores.resize(s_MaxFramesInFlight);
			s_RenderCompleteFences.resize(s_MaxFramesInFlight);

			for (uint32_t i = 0; i < s_MaxFramesInFlight; i++) 
			{
				VkSemaphoreCreateInfo semaphoreCI = {};
				semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				// Semaphore used to ensure that image acquired from swapchain before starting to submit again
				VK_CHECK(vkCreateSemaphore(s_Device, &semaphoreCI, s_Allocator, &s_ImageAcquiredSemaphores[i]));
				// Semaphore used to ensure that all commands submitted have been finished before submitting the image to the queue
				VK_CHECK(vkCreateSemaphore(s_Device, &semaphoreCI, s_Allocator, &s_RenderCompleteSemaphores[i]));

				VkFenceCreateInfo fenceCI = {};
				fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				// Create in signaled state so we don't wait on first render of each command buffer
				fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				VK_CHECK(vkCreateFence(s_Device, &fenceCI, s_Allocator, &s_RenderCompleteFences[i]));

			}
		}
	}

	static void ShutdownVulkan()
	{
		vkDeviceWaitIdle(s_Device);
		
		for (uint32_t i = 0; i < s_MaxFramesInFlight; i++)
		{
			vkDestroySemaphore(s_Device, s_ImageAcquiredSemaphores[i], s_Allocator);
			vkDestroySemaphore(s_Device, s_RenderCompleteSemaphores[i], s_Allocator);

			vkDestroyFence(s_Device, s_RenderCompleteFences[i], s_Allocator);
		}

		vkDestroyCommandPool(s_Device, s_CommandPool, s_Allocator);

		for(uint32 i = 0; i < s_MaxFramesInFlight; ++i)
			vkDestroyImageView(s_Device, s_SwapChainImageViews[i], s_Allocator);

		vkDestroySwapchainKHR(s_Device, s_SwapChain, s_Allocator);
		vkDestroySurfaceKHR(s_Instance, s_Surface, s_Allocator);

#ifdef ATN_DEBUG
		// Remove the debug report callback
		auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(s_Instance, "vkDestroyDebugReportCallbackEXT");
		vkDestroyDebugReportCallbackEXT(s_Instance, s_DebugReport, s_Allocator);
#endif
		
		vkDestroyDevice(s_Device, s_Allocator);
		vkDestroyInstance(s_Instance, s_Allocator);
	}

	static void VulkanOnUpdate()
	{
		// Acquire image from swapchain

		vkWaitForFences(s_Device, 1, &s_RenderCompleteFences[s_CurrentFrameIndex], VK_TRUE, UINT64_MAX);

		uint32 imageIndex = 0;
		VkResult result = vkAcquireNextImageKHR(s_Device, s_SwapChain, UINT64_MAX, s_ImageAcquiredSemaphores[s_CurrentFrameIndex], VK_NULL_HANDLE, &imageIndex);
		
		vkResetFences(s_Device, 1, &s_RenderCompleteFences[s_CurrentFrameIndex]);

		// Rendering
		vkResetCommandBuffer(s_CommandBuffers[s_CurrentFrameIndex], 0);

		VkCommandBufferBeginInfo cmdBufBeginInfo = {};
		cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK(vkBeginCommandBuffer(s_CommandBuffers[s_CurrentFrameIndex], &cmdBufBeginInfo));
		{
			VkClearColorValue color = { 1, 1, 0, 1 };
			VkImageSubresourceRange range;
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = 1;
			range.baseArrayLayer = 0;
			range.layerCount = 1;

			vkCmdClearColorImage(s_CommandBuffers[s_CurrentFrameIndex], s_SwapChainImages[s_CurrentFrameIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);
		}
		VK_CHECK(vkEndCommandBuffer(s_CommandBuffers[s_CurrentFrameIndex]));


		VkPipelineStageFlags waitStage[] = { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		// Submit commands to queue
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &s_ImageAcquiredSemaphores[s_CurrentFrameIndex];
		submitInfo.pWaitDstStageMask = waitStage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &s_CommandBuffers[s_CurrentFrameIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &s_RenderCompleteSemaphores[s_CurrentFrameIndex];
		VK_CHECK(vkQueueSubmit(s_Queue, 1, &submitInfo, s_RenderCompleteFences[s_CurrentFrameIndex]));

		// Present into swapchain
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &s_RenderCompleteSemaphores[s_CurrentFrameIndex];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &s_SwapChain;
		presentInfo.pImageIndices = &imageIndex;
		VK_CHECK(vkQueuePresentKHR(s_Queue, &presentInfo));

		s_CurrentFrameIndex = (s_CurrentFrameIndex + 1) % s_MaxFramesInFlight;
	}

	static void GLFWErrorCallback(int error, const char* description)
	{
		ATN_CORE_ERROR_TAG("GLFW", "Error({0}) : {1}", error, description);
	}

	static Window::WindowData& GetUserPointer(GLFWwindow* window)
	{
		return *reinterpret_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));
	}

	static void SetEventCallbacks(GLFWwindow* windowHandle)
	{
		glfwSetWindowCloseCallback(windowHandle, [](GLFWwindow* window)
			{
				Window::WindowData& data = GetUserPointer(window);

				Ref<WindowCloseEvent> event = CreateRef<WindowCloseEvent>();
				data.EventCallback(event);
			});

		glfwSetWindowSizeCallback(windowHandle, [](GLFWwindow* window, int width, int height)
			{
				Window::WindowData& data = GetUserPointer(window);
				data.Width = width;
				data.Height = height;

				Ref<WindowResizeEvent> event = CreateRef<WindowResizeEvent>(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowPosCallback(windowHandle, [](GLFWwindow* window, int xpos, int ypos)
			{
				Window::WindowData& data = GetUserPointer(window);
				
				Ref<WindowMoveEvent> event = CreateRef<WindowMoveEvent>(xpos, ypos);
				data.EventCallback(event);
			});

		glfwSetWindowFocusCallback(windowHandle, [](GLFWwindow* window, int focused)
			{
				Window::WindowData& data = GetUserPointer(window);

				if (focused)
				{
					Ref<WindowGainedFocusEvent> event = CreateRef<WindowGainedFocusEvent>();
					data.EventCallback(event);
				}
				else
				{
					Ref<WindowLostFocusEvent> event = CreateRef<WindowLostFocusEvent>();
					data.EventCallback(event);
				}
			});

		glfwSetWindowMaximizeCallback(windowHandle, [](GLFWwindow* window, int maximized)
			{
				Window::WindowData& data = GetUserPointer(window);
				if (maximized)
				{
					data.Mode = WindowMode::Maximized;

					Ref<WindowMaximizeEvent> event = CreateRef<WindowMaximizeEvent>();
					data.EventCallback(event);
				}
				else
				{
					data.Mode = WindowMode::Default;

					Ref<WindowRestoreEvent> event = CreateRef<WindowRestoreEvent>();
					data.EventCallback(event);
				}
			});

		glfwSetWindowIconifyCallback(windowHandle, [](GLFWwindow* window, int iconified)
			{
				Window::WindowData& data = GetUserPointer(window);
				if (iconified)
				{
					data.Mode = WindowMode::Minimized;

					Ref<WindowIconifyEvent> event = CreateRef<WindowIconifyEvent>();
					data.EventCallback(event);
				}
				else
				{
					data.Mode = WindowMode::Default;

					Ref<WindowRestoreEvent> event = CreateRef<WindowRestoreEvent>();
					data.EventCallback(event);
				}
			});

		glfwSetKeyCallback(windowHandle, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				Window::WindowData& data = GetUserPointer(window);
				
				bool ctrl = mods & GLFW_MOD_CONTROL;
				bool alt = mods & GLFW_MOD_ALT;
				bool shift = mods & GLFW_MOD_SHIFT;

				switch (action)
				{
				case GLFW_PRESS:
				{
					Ref<KeyPressedEvent> event = CreateRef<KeyPressedEvent>(Input::ConvertFromNativeKeyCode(key), false, ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					Ref<KeyReleasedEvent> event = CreateRef<KeyReleasedEvent>(Input::ConvertFromNativeKeyCode(key), ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					Ref<KeyPressedEvent> event = CreateRef<KeyPressedEvent>(Input::ConvertFromNativeKeyCode(key), true, ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetCharCallback(windowHandle, [](GLFWwindow* window, unsigned int character)
			{
				const auto convertFromUnicode = [](unsigned int code) { return (int)code >= 97 ? (int)code - 32 : (int)code; };

				Window::WindowData& data = GetUserPointer(window);

				Ref<KeyTypedEvent> event = CreateRef<KeyTypedEvent>(Input::ConvertFromNativeKeyCode(convertFromUnicode(character)));
				data.EventCallback(event);
			});

		glfwSetMouseButtonCallback(windowHandle, [](GLFWwindow* window, int button, int action, int mods)
			{
				Window::WindowData& data = GetUserPointer(window);

				bool ctrl = mods & GLFW_MOD_CONTROL;
				bool alt = mods & GLFW_MOD_ALT;
				bool shift = mods & GLFW_MOD_SHIFT;

				switch (action)
				{
				case GLFW_PRESS:
				{
					Ref<MouseButtonPressedEvent> event = CreateRef<MouseButtonPressedEvent>(Input::ConvertFromNativeMouseCode(button), ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					Ref<MouseButtonReleasedEvent> event = CreateRef<MouseButtonReleasedEvent>(Input::ConvertFromNativeMouseCode(button), ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				}
			});
		
		glfwSetScrollCallback(windowHandle, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				Window::WindowData& data = GetUserPointer(window);

				Ref<MouseScrolledEvent> event = CreateRef<MouseScrolledEvent>((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(windowHandle, [](GLFWwindow* window, double x, double y)
			{
				Window::WindowData& data = GetUserPointer(window);

				Ref<MouseMoveEvent> event = CreateRef<MouseMoveEvent>((float)x, (float)y);
				data.EventCallback(event);
			});

		glfwSetTitlebarHitTestCallback(windowHandle, [](GLFWwindow* window, int x, int y, int* hit)
			{
				Window::WindowData& data = GetUserPointer(window);
		
				if (data.CustomTitlebar)
				{
					*hit = data.TitlebarHitTest();
				}
			});
	}


	Scope<Window> Window::Create(const WindowCreateInfo& info)
	{
		Scope<Window> window = CreateScope<Window>();

		WindowData windowData;
		windowData.Width = info.Width;
		windowData.Height = info.Height;
		windowData.VSync = info.VSync;
		windowData.Title = info.Title;
		windowData.CustomTitlebar = info.CustomTitlebar;

		window->m_Data = windowData;

		if (m_WindowCount == 0)
		{
			int success = glfwInit();
			ATN_CORE_VERIFY(success, "Could not intialize GLFW");
			ATN_CORE_INFO_TAG("GLFW", "Init GLFW");

			glfwSetErrorCallback(GLFWErrorCallback);
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_TITLEBAR, info.CustomTitlebar ? GLFW_FALSE : GLFW_TRUE);

		GLFWwindow* glfwWindow;
		window->m_WindowHandle = glfwWindow = glfwCreateWindow(
			window->m_Data.Width,
			window->m_Data.Height,
			window->m_Data.Title.c_str(),
			nullptr,
			nullptr);

		m_WindowCount++;

		ATN_CORE_INFO_TAG("GLFW", "Create GLFW Window '{0}' ({1}, {2})", window->m_Data.Title, window->m_Data.Width, window->m_Data.Height);

		glfwSetWindowUserPointer(glfwWindow, &window->m_Data);
		SetEventCallbacks(glfwWindow);

		glfwSetWindowAttrib(glfwWindow, GLFW_RESIZABLE, info.WindowResizeable ? GLFW_TRUE : GLFW_FALSE);

		window->SetWindowMode(info.StartMode);
		window->SetIcon(info.Icon);

		// Raw mouse motion
		if (glfwRawMouseMotionSupported())
		{
			ATN_CORE_INFO_TAG("GLFW", "Raw mouse motion enabled");
			glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}
		else
		{
			ATN_CORE_WARN_TAG("GLFW", "Raw mouse motion not supported on this platform!");
		}

		// Setup Vulkan
		if (glfwVulkanSupported())
		{
			InitVulkan(glfwWindow);
		}
		else
		{
			ATN_CORE_FATAL_TAG("GLFW", "Vulkan not supported!");
		}

		//window->m_Context = CreateRef<GLGraphicsContext>(reinterpret_cast<GLFWwindow*>(window->m_WindowHandle));;
		window->SetVSync(window->m_Data.VSync);

		return window;
	}

	Window::~Window()
	{
		ShutdownVulkan();

		glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(m_WindowHandle));
		--m_WindowCount;

		ATN_CORE_INFO_TAG("GLFW", "Destroy Window '{0}'", m_Data.Title);

		if (m_WindowCount <= 0)
		{
			glfwTerminate();
			ATN_CORE_INFO_TAG("GLFW", "Shutdown GLFW");
		}
	}

	void Window::OnUpdate()
	{
		glfwPollEvents();
		VulkanOnUpdate();
		//m_Context->SwapBuffers();
	}

	void Window::SetVSync(bool enabled)
	{
		//m_Context->SetVSync(enabled);
		m_Data.VSync = enabled;
	}

	void Window::HideCursor(bool hide)
	{
		if (hide)
			glfwSetInputMode((GLFWwindow*)m_WindowHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode((GLFWwindow*)m_WindowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	void Window::SetCursorPosition(Vector2 position)
	{
		glfwSetCursorPos((GLFWwindow*)m_WindowHandle, position.x, position.y);
	}

	void Window::SetIcon(const FilePath& path)
	{
		if (FileSystem::Exists(path))
		{
			GLFWimage image;
			image.pixels = stbi_load(path.string().c_str(), &image.width, &image.height, 0, 4);
			if (image.pixels)
			{
				glfwSetWindowIcon((GLFWwindow*)m_WindowHandle, 1, &image);
				stbi_image_free(image.pixels);
			}
			else
			{
				ATN_CORE_ERROR_TAG("GLFW", "failed to load icon from '{}'!", path);
			}
		}
		else if (!path.empty())
		{
			ATN_CORE_ERROR_TAG("GLFW", "invalid filepath for icon '{}'!", path);
		}
	}

	void Window::SetWindowMode(WindowMode mode)
	{
		WindowMode currentMode = GetWindowMode();
		GLFWwindow* hWnd = reinterpret_cast<GLFWwindow*>(m_WindowHandle);
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();

		if (currentMode == mode)
			return;

		if (currentMode == WindowMode::Fullscreen)
		{
			//m_Context->SetFullscreen(false);
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			glfwSetWindowMonitor(hWnd, nullptr, 0, 0, m_Data.Width, m_Data.Width, mode->refreshRate);
		}

		switch (mode)
		{
		case WindowMode::Default:
		{
			glfwRestoreWindow(hWnd);
			break;
		}
		case WindowMode::Maximized:
		{
			glfwMaximizeWindow(hWnd);
			break;
		}
		case WindowMode::Minimized:
		{
			glfwIconifyWindow(hWnd);
			break;
		}
		case WindowMode::Fullscreen:
		{
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			
			glfwSetWindowMonitor(hWnd, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			//m_Context->SetFullscreen(true);
			break;
		}
		}

		m_Data.Mode = mode;
	}
}
