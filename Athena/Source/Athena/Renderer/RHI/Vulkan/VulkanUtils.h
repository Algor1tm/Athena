#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/ConsoleManager.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanCommandContext.h"

#include <vulkan/vulkan.h>


namespace Athena::Vulkan
{
	inline VkDevice GetLogicalDevice()
	{
		return VulkanRHI::Get()->GetDevice()->GetLogicalDevice();
	}

    inline Ref<VulkanAllocator> GetAllocator()
    {
        return VulkanRHI::Get()->GetDevice()->GetAllocator();
    }

    inline uint32 GetMaxFramesInFlight()
    {
        return ConsoleManager::Get().FindCVar("r.MaxFramesInFlight")->GetInt();
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
		T procedure = reinterpret_cast<T>(vkGetDeviceProcAddr(GetLogicalDevice(), procName));
		ensuref(procedure != NULL);

		return procedure;
	}

    static uint32 GetLayerMultiplier(TextureType type)
    {
        if (type == TextureType::TEXTURE_CUBE)
            return 6;

        return 1;
    }

    inline VkFormat TranslateFormat(Format format)
    {
        switch (format)
        {
        case Format::R8:              return VK_FORMAT_R8_UNORM;
        case Format::RG8:             return VK_FORMAT_R8G8_UNORM;
        case Format::RGBA8:           return VK_FORMAT_R8G8B8A8_UNORM;
        case Format::RGBA8_SRGB:      return VK_FORMAT_R8G8B8A8_SRGB;
        case Format::R10G10B10A2:     return VK_FORMAT_A2R10G10B10_UNORM_PACK32;

        case Format::R32F:            return VK_FORMAT_R32_SFLOAT;
        case Format::RG16F:           return VK_FORMAT_R16G16_SFLOAT;
        case Format::RG32F:           return VK_FORMAT_R32G32_SFLOAT;
        case Format::R11G11B10UF:     return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case Format::RGB32F:          return VK_FORMAT_R32G32B32_SFLOAT;
        case Format::RGBA16F:         return VK_FORMAT_R16G16B16A16_SFLOAT;
        case Format::RGBA32F:         return VK_FORMAT_R32G32B32A32_SFLOAT;
        case Format::RGBA32UInt:      return VK_FORMAT_R32G32B32A32_UINT;

        case Format::DEPTH16:         return VK_FORMAT_D16_UNORM;
        case Format::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
        case Format::DEPTH32F:        return VK_FORMAT_D32_SFLOAT;
        }

        checkf(false);
        return (VkFormat)0;
    }

    inline VkImageViewType TranslateImageViewType(TextureType type, uint32 layersCount)
    {
        if (type == TextureType::TEXTURE_2D)
        {
            if (layersCount == 1)
                return VK_IMAGE_VIEW_TYPE_2D;

            else if (layersCount > 1)
                return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        }
        else if (type == TextureType::TEXTURE_CUBE)
        {
            if (layersCount == 1)
                return VK_IMAGE_VIEW_TYPE_2D;

            else if (layersCount == 6)
                return VK_IMAGE_VIEW_TYPE_CUBE;

            else if (layersCount > 6 && layersCount % 6 == 0)
                return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        }

        checkf(false);
        return (VkImageViewType)0;
    }

    inline VkComponentSwizzle TranslateSwizzle(ComponentMapping swizzle)
    {
        switch (swizzle)
        {
        case ComponentMapping::IDENTITY: return VK_COMPONENT_SWIZZLE_IDENTITY;
        case ComponentMapping::ZERO:     return VK_COMPONENT_SWIZZLE_ZERO;
        case ComponentMapping::ONE:      return VK_COMPONENT_SWIZZLE_ONE;
        case ComponentMapping::R:        return VK_COMPONENT_SWIZZLE_R;
        case ComponentMapping::G:        return VK_COMPONENT_SWIZZLE_G;
        case ComponentMapping::B:        return VK_COMPONENT_SWIZZLE_B;
        case ComponentMapping::A:        return VK_COMPONENT_SWIZZLE_A;
        }

        checkf(false);
        return VK_COMPONENT_SWIZZLE_IDENTITY;
    }

    inline VkImageAspectFlags TranslateTextureAspect(TexturePlane plane, Format format)
    {
        const auto& formatProps = GetFormatProperties(format);

        VkImageAspectFlags result = 0;

        if (formatProps.HasColor)
        {
            result |= VK_IMAGE_ASPECT_COLOR_BIT;
        }
        else
        {
            if (formatProps.HasDepth)
            {
                if (plane == TexturePlane::ALL || plane == TexturePlane::PLANE_0)
                    result |= VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            if (formatProps.HasStencil)
            {
                if (plane == TexturePlane::ALL || plane == TexturePlane::PLANE_1)
                    result |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }

        return result;
    }
}
