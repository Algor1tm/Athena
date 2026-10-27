#include "VulkanRHI.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/ConsoleManager.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanDebug.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanPlatform.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanBuffers.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanCommandList.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanTexture.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanTextureView.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanSwapChain.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanImGuiBackend.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanTracyZone.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	static AutoCVar<int32> CVarVulkanMajorVersion(
		"Vulkan.MajorVersion",
		1,
		"Sets vulkan major version, set -1 to use latest available."
	);

	static AutoCVar<int32> CVarVulkanMinorVersion(
		"Vulkan.MinorVersion",
		3,
		"Sets vulkan minor version, set -1 to use latest available."
	);

	static AutoCVar<int32> CVarVulkanPatchVersion(
		"Vulkan.PatchVersion",
		-1,
		"Sets vulkan patch version, set -1 to use latest available."
	);

#if VULKAN_ENABLE_DEBUG_INFO
	static AutoCVar<int32> CVarVulkanValidationLevel(
		"Vulkan.ValidationLevel",
		2,
		"Sets debug report level 0 - 4 (0 - disable validation, default - 2)."
	);
#endif

#define VK_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

	static const char* s_RequiredVulkanInstanceExtensions[] =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		nullptr
	};

	static const char* s_OptionalVulkanInstanceExtensions[] =
	{
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,

#if VULKAN_ENABLE_DEBUG_INFO
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
		nullptr
	};

	static const char* s_OptionalVulkanInstanceLayers[] =
	{
		nullptr
	};

	static bool IsExtensionOrLayerPresent(const std::vector<const char*>& extensions, const char* extension)
	{
		for (uint32 j = 0; j < extensions.size(); ++j)
		{
			if (!strcmp(extension, extensions[j]))
			{
				return true;
			}
		}

		return false;
	}

	static uint32 SelectVulkanVersion()
	{
		uint32 latestVersion = 0;
		VK_CHECK(vkEnumerateInstanceVersion(&latestVersion));

		const uint32 supportedVariant = VK_API_VERSION_VARIANT(latestVersion);
		const uint32 supportedMajor = VK_API_VERSION_MAJOR(latestVersion);
		const uint32 supportedMinor = VK_API_VERSION_MINOR(latestVersion);
		const uint32 supportedPatch = VK_API_VERSION_PATCH(latestVersion);

		int32 requestedVariant = 0;
		int32 requestedMajor = CVarVulkanMajorVersion.GetInt();
		int32 requestedMinor = CVarVulkanMinorVersion.GetInt();
		int32 requestedPatch = CVarVulkanPatchVersion.GetInt();

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

	void VulkanRHI::Init(const RendererConfig& config)
	{
		m_VulkanVersion = SelectVulkanVersion();

		const uint32 variant = VK_API_VERSION_VARIANT(m_VulkanVersion);
		const uint32 major = VK_API_VERSION_MAJOR(m_VulkanVersion);
		const uint32 minor = VK_API_VERSION_MINOR(m_VulkanVersion);
		const uint32 patch = VK_API_VERSION_PATCH(m_VulkanVersion);

		ATN_LOG_INFO(Vulkan, "Selected Vulkan API version: {}.{}.{}.{}", variant, major, minor, patch);

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = nullptr;
		appInfo.pApplicationName = Application::Get().GetConfig().Name.c_str();
		appInfo.pEngineName = "Athena";
		appInfo.apiVersion = m_VulkanVersion;

		QueryInstanceLayers();
		QueryInstanceExtensions();

		// Create Vulkan Instance
		VkInstanceCreateInfo instanceCI = {};
		instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCI.pNext = nullptr;
		instanceCI.flags = 0;
		instanceCI.pApplicationInfo = &appInfo;
		instanceCI.enabledLayerCount = m_InstanceLayers.size();
		instanceCI.ppEnabledLayerNames = m_InstanceLayers.data();
		instanceCI.enabledExtensionCount = m_InstanceExtensions.size();
		instanceCI.ppEnabledExtensionNames = m_InstanceExtensions.data();

		VK_CHECK(vkCreateInstance(&instanceCI, nullptr, &m_Instance));
		
		PostInstanceInit();

		m_Device.Initialize(m_Instance);

		m_ResourceFreeQueues.resize(Vulkan::GetMaxFramesInFlight());
		for (uint32 i = 0; i < m_ResourceFreeQueues.size(); ++i)
		{
			m_ResourceFreeQueues[i] = CommandStream(1024 * 1024);	// 1 Mb
		}

		m_FrameResourcesIndex = 0;

		m_Allocator = Ref<VulkanAllocator>::Create();
		m_CmdBufferManager.Init();

		ATN_LOG_INFO(Vulkan, "Successfully initialized Vulkan RHI");
	}

	void VulkanRHI::Shutdown()
	{
		vkDeviceWaitIdle(Vulkan::GetLogicalDevice());

		for (auto& queue : m_ResourceFreeQueues)
		{
			queue.Flush();
		}

		m_CmdBufferManager.Destroy();
		m_Allocator.Release();
		m_Device.Destroy();

#if VULKAN_ENABLE_DEBUG_INFO
		if (m_ExtensionsInfo.DebugUtilsEXTSupported)
		{
			PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = Vulkan::GetInstanceProc<PFN_vkDestroyDebugUtilsMessengerEXT>("vkDestroyDebugUtilsMessengerEXT");
			vkDestroyDebugUtilsMessengerEXT(m_Instance, m_ExtensionsInfo.DebugUtilsMessenger, nullptr);
		}
#endif

		vkDestroyInstance(m_Instance, nullptr);
	}

	void VulkanRHI::BeginFrame(const Ref<Window>& window)
	{
		TRACY_PROFILE_FUNC();

		static uint32 maxFramesInFlight = ConsoleManager::Get().FindCVar("r.MaxFramesInFlight")->GetInt();
		m_FrameResourcesIndex = (m_FrameResourcesIndex + 1) % maxFramesInFlight;

		if(window->GetSwapChain())
			window->GetSwapChain().As<VulkanSwapChain>()->AcquireImage();

		{
			TRACY_PROFILE_SCOPE("ResourceFreeQueue::Flush");
			m_ResourceFreeQueues[m_FrameResourcesIndex].Flush();
		}

		m_Allocator->NewFrame();
		m_CmdBufferManager.NewFrame();
	}

	void VulkanRHI::EndFrame(const Ref<Window>& window)
	{
		TRACY_PROFILE_FUNC();

		// If there is no ImGui we need to manually render to swapchain images
		if (!Application::Get().GetConfig().EnableImGui)
		{
			// Blit to swapchain final image
		}

		if (window->GetSwapChain())
			window->GetSwapChain().As<VulkanSwapChain>()->Present();
	}

	Ref<Texture> VulkanRHI::CreateTexture(const TextureCreateInfo& info)
	{
		return Ref<VulkanTexture>::Create(info);
	}

	Ref<TextureView> VulkanRHI::CreateTextureView(const TextureViewCreateInfo& info, Texture* texture)
	{
		return Ref<VulkanTextureView>::Create(info, texture);
	}

	Ref<VertexBuffer> VulkanRHI::CreateVertexBuffer(const String& name, uint64 size)
	{
		return Ref<VulkanVertexBuffer>::Create(name, size);
	}

	Ref<IndexBuffer> VulkanRHI::CreateIndexBuffer(const String& name, uint64 size, IndexSize indexSize)
	{
		return Ref<VulkanIndexBuffer>::Create(name, size, indexSize);
	}

	Ref<CommandList> VulkanRHI::CreateCommandList()
	{
		return Ref<VulkanCommandList>::Create();
	}

	Ref<TracyGPUZone> VulkanRHI::CreateTracyZone(const Ref<CommandList>& cmdList, uint32 line, std::string_view fileName, std::string_view functionName, const String& name)
	{
		return Ref<VulkanTracyZone>::Create(cmdList, line, fileName, functionName, name);
	}

	Ref<SwapChain> VulkanRHI::CreateSwapChain(void* windowHandle, uint32 width, uint32 height, bool vsync)
	{
		return Ref<VulkanSwapChain>::Create(windowHandle, vsync);
	}

	Ref<ImGuiBackend> VulkanRHI::CreateImGuiBackend()
	{
		return Ref<VulkanImGuiBackend>::Create();
	}

	uint64 VulkanRHI::GetMemoryUsage()
	{
		return m_Allocator->GetMemoryUsage();
	}

	const GPUCapabilities& VulkanRHI::GetGPUCapabilities()
	{
		return m_Device.GetGPUCapabilities();
	}

	const FormatSupportProperties& VulkanRHI::GetFormatSupportProperties(Format format)
	{
		return m_Device.GetGPUCapabilities().Formats[format];
	}

	void VulkanRHI::PostInstanceInit()
	{
#if VULKAN_ENABLE_DEBUG_INFO
		m_ExtensionsInfo.DebugUtilsEXTSupported = IsExtensionOrLayerPresent(m_InstanceExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		if (m_ExtensionsInfo.DebugUtilsEXTSupported)
		{
			PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugReportCallbackEXT = Vulkan::GetInstanceProc<PFN_vkCreateDebugUtilsMessengerEXT>("vkCreateDebugUtilsMessengerEXT");

			VkDebugUtilsMessengerCreateInfoEXT callbackInfo = {};
			callbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			callbackInfo.flags = 0;
			callbackInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			callbackInfo.pfnUserCallback = Vulkan::DebugCallback;

			int32 CVar = CVarVulkanValidationLevel.GetInt();
			switch (CVar)
			{
			default:
				callbackInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
			case 3:
				callbackInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
			case 2:
				callbackInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
			case 1:
				callbackInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			case 0:
				break;
			}

			VK_CHECK(vkCreateDebugReportCallbackEXT(m_Instance, &callbackInfo, NULL, &m_ExtensionsInfo.DebugUtilsMessenger));

			m_ExtensionsInfo.vkSetDebugUtilsObjectNameEXT = Vulkan::GetInstanceProc<PFN_vkSetDebugUtilsObjectNameEXT>("vkSetDebugUtilsObjectNameEXT");
			m_ExtensionsInfo.vkCmdBeginDebugUtilsLabelEXT = Vulkan::GetInstanceProc<PFN_vkCmdBeginDebugUtilsLabelEXT>("vkCmdBeginDebugUtilsLabelEXT");
			m_ExtensionsInfo.vkCmdEndDebugUtilsLabelEXT = Vulkan::GetInstanceProc<PFN_vkCmdEndDebugUtilsLabelEXT>("vkCmdEndDebugUtilsLabelEXT");
			m_ExtensionsInfo.vkCmdInsertDebugUtilsLabelEXT = Vulkan::GetInstanceProc<PFN_vkCmdInsertDebugUtilsLabelEXT>("vkCmdInsertDebugUtilsLabelEXT");
			m_ExtensionsInfo.vkQueueBeginDebugUtilsLabelEXT = Vulkan::GetInstanceProc<PFN_vkQueueBeginDebugUtilsLabelEXT>("vkQueueBeginDebugUtilsLabelEXT");
			m_ExtensionsInfo.vkQueueEndDebugUtilsLabelEXT = Vulkan::GetInstanceProc<PFN_vkQueueEndDebugUtilsLabelEXT>("vkQueueEndDebugUtilsLabelEXT");
		}
#endif

		m_ExtensionsInfo.GetPhysicalDeviceProperties2EXTSupported = IsExtensionOrLayerPresent(m_InstanceExtensions, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	}

	void VulkanRHI::QueryInstanceLayers()
	{
		uint32 supportedLayerCount = 0;
		vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);

		std::vector<VkLayerProperties> supportedLayers(supportedLayerCount);
		vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.data());

		String message = "Found vulkan instance layers: \n\t";
		for (auto layer : supportedLayers)
			message += fmt::format("'{}'\n\t", layer.layerName);

		ATN_LOG_INFO(Vulkan, message);

		std::vector<const char*> layers;
		for (uint32 i = 0; i < ATN_ARRAY_SIZE(s_OptionalVulkanInstanceLayers); ++i)
		{
			layers.push_back(s_OptionalVulkanInstanceLayers[i]);
		}

#if VULKAN_ENABLE_DEBUG_INFO
		bool enableDebugLayer = CVarVulkanValidationLevel.GetInt() > 0;

		if (enableDebugLayer)
			layers.push_back(VK_VALIDATION_LAYER_NAME);
#endif

		for (uint32 i = 0; i < layers.size(); ++i)
		{
			if (!layers[i])
				continue;

			bool found = false;
			for (uint32 j = 0; j < supportedLayers.size(); ++j)
			{
				if (!strcmp(layers[i], supportedLayers[j].layerName))
				{
					found = true;
					break;
				}
			}

			if (found)
				m_InstanceLayers.push_back(layers[i]);
		}

		message = "Vulkan using instance layers: \n\t";
		for (auto ext : m_InstanceLayers)
			message += fmt::format("'{}'\n\t", ext);

		ATN_LOG_INFO(Vulkan, message);

		if (IsExtensionOrLayerPresent(m_InstanceLayers, VK_VALIDATION_LAYER_NAME))
		{
			ATN_LOG_INFO(Vulkan, "Vulkan debug layer is enabled");
		}
	}

	void VulkanRHI::QueryInstanceExtensions()
	{
		uint32 supportedExtensionCount = 0;
		vkEnumerateInstanceExtensionProperties(NULL, &supportedExtensionCount, nullptr);

		std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
		vkEnumerateInstanceExtensionProperties(NULL, &supportedExtensionCount, supportedExtensions.data());

		String message = "Found vulkan instance extensions: \n\t";
		for (auto ext : supportedExtensions)
			message += fmt::format("'{}'\n\t", ext.extensionName);

		ATN_LOG_INFO(Vulkan, message);

		std::vector<const char*> missingExtensions;
		std::vector<const char*> platformExtensions = VulkanPlatform::GetRequiredInstanceExtensions();

		for (uint32 i = 0; i < ATN_ARRAY_SIZE(s_RequiredVulkanInstanceExtensions); ++i)
		{
			if (!s_RequiredVulkanInstanceExtensions[i])
				continue;

			bool found = false;
			for (uint32 j = 0; j < supportedExtensions.size(); ++j)
			{
				if (!strcmp(s_RequiredVulkanInstanceExtensions[i], supportedExtensions[j].extensionName))
				{
					found = true;
					break;
				}
			}

			if (found)
				m_InstanceExtensions.push_back(s_RequiredVulkanInstanceExtensions[i]);
			else
				missingExtensions.push_back(s_RequiredVulkanInstanceExtensions[i]);
		}


		for (uint32 i = 0; i < platformExtensions.size(); ++i)
		{
			if (!platformExtensions[i])
				continue;

			bool found = false;
			for (uint32 j = 0; j < supportedExtensions.size(); ++j)
			{
				if (!strcmp(platformExtensions[i], supportedExtensions[j].extensionName))
				{
					found = true;
					break;
				}
			}

			if (found)
				m_InstanceExtensions.push_back(platformExtensions[i]);
			else
				missingExtensions.push_back(platformExtensions[i]);
		}


		for (uint32 i = 0; i < ATN_ARRAY_SIZE(s_OptionalVulkanInstanceExtensions); ++i)
		{
			if (!s_OptionalVulkanInstanceExtensions[i])
				continue;

			bool found = false;
			for (uint32 j = 0; j < supportedExtensions.size(); ++j)
			{
				if (s_OptionalVulkanInstanceExtensions[i] && !strcmp(s_OptionalVulkanInstanceExtensions[i], supportedExtensions[j].extensionName))
				{
					found = true;
					break;
				}
			}

			if (found)
				m_InstanceExtensions.push_back(s_OptionalVulkanInstanceExtensions[i]);
		}

		if (!missingExtensions.empty())
		{
			message = "Missing instance extensions: \n\t";
			for (auto ext : missingExtensions)
				message += fmt::format("'{}'\n\t", ext);

			ATN_LOG_FATAL(Vulkan, message);
		}

		message = "Vulkan using instance extensions: \n\t";
		for (auto ext : m_InstanceExtensions)
			message += fmt::format("'{}'\n\t", ext);

		ATN_LOG_INFO(Vulkan, message);
	}
}
