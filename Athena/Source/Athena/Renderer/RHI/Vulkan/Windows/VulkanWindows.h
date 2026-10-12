#pragma once

#include "Athena/Renderer/RHI/Vulkan/VulkanPlatform.h"

#include <Windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


namespace Athena
{
	std::vector<const char*> VulkanPlatform::GetRequiredInstanceExtensions()
	{
		return std::vector<const char*>(
		{
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		});
	}

	std::vector<const char*> VulkanPlatform::GetRequiredDeviceExtensions()
	{
		return std::vector<const char*>(
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		});
	}
}
