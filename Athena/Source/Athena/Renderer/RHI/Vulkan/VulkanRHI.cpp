#include "VulkanRHI.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/ConsoleManager.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanDebug.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanPlatform.h"

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
		"Sets debug report level 0 - 5 (0 - disable validation, default - 2)."
	);

#endif

	static const char* s_RequiredVulkanInstanceExtensions[] =
	{
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		nullptr
	};

	static const char* s_OptionalVulkanInstanceExtensions[] =
	{
#if VULKAN_ENABLE_DEBUG_INFO
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
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
		
#if VULKAN_ENABLE_DEBUG_INFO
		if (m_OptionalExtensionsInfo.DebugReportEXTSupported)
		{
			// Setup the debug report callback
			PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = Vulkan::GetInstanceProc<PFN_vkCreateDebugReportCallbackEXT>("vkCreateDebugReportCallbackEXT");

			VkDebugReportCallbackCreateInfoEXT reportCI = {};
			reportCI.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			reportCI.flags = 0;
			reportCI.pfnCallback = Vulkan::DebugCallback;
			reportCI.pUserData = NULL;

			int32 CVar = CVarVulkanValidationLevel.GetInt();
			switch (CVar)
			{
			default:
				reportCI.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
			case 4:
				reportCI.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
			case 3:
				reportCI.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			case 2:
				reportCI.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
			case 1:
				reportCI.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
				break;
			case 0:
				break;
			}

			VK_CHECK(vkCreateDebugReportCallbackEXT(m_Instance, &reportCI, nullptr, &m_OptionalExtensionsInfo.DebugReportCallback));
		}
#endif

		m_Device.Initialize(m_Instance);

		ATN_LOG_INFO(Vulkan, "Successfully initialized Vulkan RHI");
	}

	void VulkanRHI::Shutdown()
	{
		m_Device.Destroy();

#if VULKAN_ENABLE_DEBUG_INFO
		if (m_OptionalExtensionsInfo.DebugReportEXTSupported)
		{
			PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = Vulkan::GetInstanceProc<PFN_vkDestroyDebugReportCallbackEXT>("vkDestroyDebugReportCallbackEXT");
			vkDestroyDebugReportCallbackEXT(m_Instance, m_OptionalExtensionsInfo.DebugReportCallback, nullptr);
		}
#endif

		vkDestroyInstance(m_Instance, nullptr);
	}

	const GPUCapabilities& VulkanRHI::GetGPUCapabilities()
	{
		return m_Device.GetGPUCapabilities();
	}

	const FormatProperties& VulkanRHI::GetFormatProperties(Format format)
	{
		return m_Device.GetGPUCapabilities().Formats[format];
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
			layers.push_back("VK_LAYER_KHRONOS_validation");
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

		if (IsExtensionOrLayerPresent(m_InstanceLayers, "VK_LAYER_KHRONOS_validation"))
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

#if VULKAN_ENABLE_DEBUG_INFO
		m_OptionalExtensionsInfo.DebugReportEXTSupported = IsExtensionOrLayerPresent(m_InstanceExtensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif
	}
}
