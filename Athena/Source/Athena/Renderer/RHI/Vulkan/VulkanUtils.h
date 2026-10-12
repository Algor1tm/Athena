#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"

#include <vulkan/vulkan.h>


namespace Athena::Vulkan
{
	inline VkDevice GetDevice()
	{
		return VulkanRHI::Get()->GetDevice()->GetLogicalDevice();
	}

	template<typename T>
	inline T GetInstanceProc(const char* procName)
	{
		T procedure = reinterpret_cast<T>(vkGetInstanceProcAddr(VulkanRHI::Get()->GetVulkanInstance(), procName));
		ensuref(procedure != NULL);

		return procedure;
	}

	template<typename T>
	inline T GetDeviceProc(const char* procName)
	{
		T procedure = reinterpret_cast<T>(vkGetDeviceProcAddr(GetDevice(), procName));
		ensuref(procedure != NULL);

		return procedure;
	}

    inline VkFormat TranslateFormat(Format format)
    {
        switch (format)
        {
        case Format::R8:              return VK_FORMAT_R8_UNORM;
        case Format::R8_SRGB:         return VK_FORMAT_R8_SRGB;
        case Format::RG8:             return VK_FORMAT_R8G8_UNORM;
        case Format::RG8_SRGB:        return VK_FORMAT_R8G8_SRGB;
        case Format::RGB8:            return VK_FORMAT_R8G8B8_UNORM;
        case Format::RGB8_SRGB:       return VK_FORMAT_R8G8B8_SRGB;
        case Format::RGBA8:           return VK_FORMAT_R8G8B8A8_UNORM;
        case Format::RGBA8_SRGB:      return VK_FORMAT_R8G8B8A8_SRGB;

        case Format::R32F:            return VK_FORMAT_R32_SFLOAT;
        case Format::RG16F:           return VK_FORMAT_R16G16_SFLOAT;
        case Format::R11G11B10F:      return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case Format::RGB16F:          return VK_FORMAT_R16G16B16_SFLOAT;
        case Format::RGB32F:          return VK_FORMAT_R32G32B32_SFLOAT;
        case Format::RGBA16F:         return VK_FORMAT_R16G16B16A16_SFLOAT;
        case Format::RGBA32F:         return VK_FORMAT_R32G32B32A32_SFLOAT;

        case Format::DEPTH16:         return VK_FORMAT_D16_UNORM;
        case Format::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
        case Format::DEPTH32F:        return VK_FORMAT_D32_SFLOAT;
        }

        checkf(false);
        return (VkFormat)0;
    }
}
