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
	static std::vector<VkImage>		s_SwapChainImages;

	static VkDescriptorPool         s_DescriptorPool = VK_NULL_HANDLE;

	static VkDebugReportCallbackEXT s_DebugReport = VK_NULL_HANDLE;
	 
	static uint32					s_FramesInFlight = 2;

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

#define CHECK_VK_RESULT(expr) check_vk_result(expr)

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
		VkResult result;
		// Create Instance
		{
			// Select Vulkan Version
			uint32 version = 0;
			result = vkEnumerateInstanceVersion(&version);
			CHECK_VK_RESULT(result);

			uint32 variant = VK_API_VERSION_VARIANT(version);
			uint32 major = VK_API_VERSION_MAJOR(version);
			uint32 minor = VK_API_VERSION_MINOR(version);
			uint32 patch = VK_API_VERSION_PATCH(version);

			ATN_CORE_INFO_TAG("Vulkan", "Version: {}.{}.{}.{}", variant, major, minor, patch);

			VkApplicationInfo appInfo;
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
			VkInstanceCreateInfo instCreateInfo;
			instCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instCreateInfo.pNext = nullptr;
			instCreateInfo.flags = 0;
			instCreateInfo.pApplicationInfo = &appInfo;
			instCreateInfo.enabledLayerCount = layers.size();
			instCreateInfo.ppEnabledLayerNames = layers.data();
			instCreateInfo.enabledExtensionCount = extensions.size();
			instCreateInfo.ppEnabledExtensionNames = extensions.data();

			result = vkCreateInstance(&instCreateInfo, s_Allocator, &s_Instance);
			CHECK_VK_RESULT(result);

#ifdef ATN_DEBUG
			// Setup the debug report callback
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(s_Instance, "vkCreateDebugReportCallbackEXT");
			ATN_CORE_ASSERT(vkCreateDebugReportCallbackEXT != NULL);

			VkDebugReportCallbackCreateInfoEXT reportCreateInfo = {};
			reportCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			reportCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			reportCreateInfo.pfnCallback = VulkanDebugCallback;
			reportCreateInfo.pUserData = NULL;
			result = vkCreateDebugReportCallbackEXT(s_Instance, &reportCreateInfo, s_Allocator, &s_DebugReport);
			CHECK_VK_RESULT(result);
#endif
		}

		// Select GPU
		{
			uint32 gpu_count;
			result = vkEnumeratePhysicalDevices(s_Instance, &gpu_count, NULL);
			CHECK_VK_RESULT(result);

			std::vector<VkPhysicalDevice> gpus(gpu_count);

			result = vkEnumeratePhysicalDevices(s_Instance, &gpu_count, gpus.data());
			CHECK_VK_RESULT(result);

			String message = "GPUs: \n\t";
			String selectedGPUName;

			uint32 use_gpu = 0;
			for (uint32 i = 0; i < gpu_count; i++)
			{
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(gpus[i], &properties);
				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					use_gpu = i;
					selectedGPUName = properties.deviceName;
				}

				message += std::format("{}\n\t", properties.deviceName);
			}

			ATN_CORE_INFO_TAG("Vulkan", message);
			ATN_CORE_INFO_TAG("Vulkan", "Selected GPU: {}\n", selectedGPUName);

			s_PhysicalDevice = gpus[use_gpu];
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

			VkDeviceQueueCreateInfo queueInfos[1] = {};
			queueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfos[0].queueFamilyIndex = s_QueueFamily;
			queueInfos[0].queueCount = 1;
			queueInfos[0].pQueuePriorities = queuePriority;

			message += std::format("QueueFamily - {}, count - {}\n\t", s_QueueFamily, 1);
			ATN_CORE_INFO_TAG("Vulkan", message);

			std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

			message = "Device extensions: \n\t";
			for (auto ext : deviceExtensions)
				message += std::format("{}\n\t", ext);

			ATN_CORE_INFO_TAG("Vulkan", message);

			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.queueCreateInfoCount = std::size(queueInfos);
			createInfo.pQueueCreateInfos = queueInfos;
			createInfo.enabledExtensionCount = deviceExtensions.size();
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();

			result = vkCreateDevice(s_PhysicalDevice, &createInfo, s_Allocator, &s_Device);
			CHECK_VK_RESULT(result);
			vkGetDeviceQueue(s_Device, s_QueueFamily, 0, &s_Queue);
		}

		// Create window surface and SwapChain
		{
			result = glfwCreateWindowSurface(s_Instance, window, s_Allocator, &s_Surface);
			CHECK_VK_RESULT(result);

			// Check for WSI support
			VkBool32 res;
			vkGetPhysicalDeviceSurfaceSupportKHR(s_PhysicalDevice, s_QueueFamily, s_Surface, &res);
			ATN_CORE_VERIFY(res, "no WSI support on physical device");

			ATN_CORE_INFO_TAG("Vulkan", "Create Window Surface");

			VkSurfaceCapabilitiesKHR surfaceCapabilities;
			std::vector<VkSurfaceFormatKHR> surfaceFormats;
			std::vector<VkPresentModeKHR> surfacePresentModes;

			// Surface Capabilites
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s_PhysicalDevice, s_Surface, &surfaceCapabilities);

			ATN_CORE_VERIFY(surfaceCapabilities.minImageCount <= s_FramesInFlight && surfaceCapabilities.maxImageCount >= s_FramesInFlight);

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
			int32 useFormat = -1;
			for (uint64 i = 0; i < surfaceFormats.size(); ++i)
			{
				if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					useFormat = i;
					break;
				}
			}

			ATN_CORE_VERIFY(useFormat != -1, "Failed to find supported window surface format!");
			VkSurfaceFormatKHR selectedFormat = surfaceFormats[useFormat];

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

			Window::WindowData& windowData = *(Window::WindowData*)(glfwGetWindowUserPointer(window));
			uint32 width = windowData.Width;
			uint32 height = windowData.Height;

			VkSwapchainCreateInfoKHR info;
			info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			info.surface = s_Surface;
			info.minImageCount = s_FramesInFlight;
			info.imageFormat = selectedFormat.format;
			info.imageColorSpace = selectedFormat.colorSpace;
			info.imageExtent.width = width;
			info.imageExtent.height = height;
			info.imageArrayLayers = 1;
			info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;           // Assume that graphics family == present family
			info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			info.presentMode = selectedPresentMode;
			info.clipped = VK_TRUE;
			info.oldSwapchain = s_SwapChain;

			// Crashes with no debug message
			
			//result = vkCreateSwapchainKHR(s_Device, &info, s_Allocator, &s_SwapChain); 
			//CHECK_VK_RESULT(result);
			//
			//s_SwapChainImages.resize(s_FramesInFlight);
			//result = vkGetSwapchainImagesKHR(s_Device, s_SwapChain, &s_FramesInFlight, s_SwapChainImages.data());
			//CHECK_VK_RESULT(result);
		}

		// Create Descriptor Pool
		{
			VkDescriptorPoolSize poolSizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};

			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 1000 * std::size(poolSizes);
			pool_info.poolSizeCount = std::size(poolSizes);
			pool_info.pPoolSizes = poolSizes;

			result = vkCreateDescriptorPool(s_Device, &pool_info, s_Allocator, &s_DescriptorPool);
			CHECK_VK_RESULT(result);
		}
	}

	static void ShutdownVulkan()
	{
		vkQueueWaitIdle(s_Queue);

		vkDestroyDescriptorPool(s_Device, s_DescriptorPool, s_Allocator);

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

		// Icon
		if (FileSystem::Exists(info.Icon))
		{
			GLFWimage image;
			image.pixels = stbi_load(info.Icon.string().c_str(), &image.width, &image.height, 0, 4);
			if (image.pixels)
			{
				glfwSetWindowIcon(glfwWindow, 1, &image);
				stbi_image_free(image.pixels);
			}
			else
			{
				ATN_CORE_ERROR_TAG("GLFW", "failed to load icon from {}!", info.Icon);
			}
		}
		else if(!info.Icon.empty())
		{
			ATN_CORE_ERROR_TAG("GLFW", "invalid filepath for icon '{}'!", info.Icon);
		}

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
