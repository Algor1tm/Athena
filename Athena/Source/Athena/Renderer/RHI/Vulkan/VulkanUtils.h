#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/ConsoleManager.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/CommandBufferManager.h"

#include <vulkan/vulkan.h>


namespace Athena::Vulkan
{
	inline VkDevice GetLogicalDevice()
	{
		return VulkanRHI::Get()->GetDevice()->GetLogicalDevice();
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

    inline VkComponentSwizzle TranslateSwizzle(ComponentSwizzle swizzle)
    {
        switch (swizzle)
        {
        case ComponentSwizzle::IDENTITY: return VK_COMPONENT_SWIZZLE_IDENTITY;
        case ComponentSwizzle::ZERO:     return VK_COMPONENT_SWIZZLE_ZERO;
        case ComponentSwizzle::ONE:      return VK_COMPONENT_SWIZZLE_ONE;
        case ComponentSwizzle::R:        return VK_COMPONENT_SWIZZLE_R;
        case ComponentSwizzle::G:        return VK_COMPONENT_SWIZZLE_G;
        case ComponentSwizzle::B:        return VK_COMPONENT_SWIZZLE_B;
        case ComponentSwizzle::A:        return VK_COMPONENT_SWIZZLE_A;
        }

        checkf(false);
        return VK_COMPONENT_SWIZZLE_IDENTITY;
    }

    inline VkImageAspectFlagBits TranslateTextureAspect(TextureAspect aspect, Format format)
    {
        if (aspect == TextureAspect::AUTO)
            aspect = GetAspectFromFormat(format);

        switch (aspect)
        {
        case TextureAspect::COLOR: return VK_IMAGE_ASPECT_COLOR_BIT;
        case TextureAspect::DEPTH: return VK_IMAGE_ASPECT_DEPTH_BIT;
        case TextureAspect::STENCIL: return VK_IMAGE_ASPECT_STENCIL_BIT;
        case TextureAspect::DEPTH_STENCIL: return VkImageAspectFlagBits(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
        }

        checkf(false);
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }

    inline void BlitMipMap(VulkanCommandBuffer* commandBuffer, VkImage image, uint32 width, uint32 height, uint32 layers, Format format, uint32 mipLevels)
    {
        int32 mipWidth = width;
        int32 mipHeight = height;

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = Vulkan::TranslateTextureAspect(TextureAspect::AUTO, format);
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layers;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        for (uint32 level = 1; level < mipLevels; ++level)
        {
            bool firstMip = level == 1;

            barrier.subresourceRange.baseMipLevel = level - 1;
            barrier.oldLayout = firstMip ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = firstMip ? VK_ACCESS_NONE : VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

            vkCmdPipelineBarrier(
                commandBuffer->CommandBuffer,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            barrier.subresourceRange.baseMipLevel = level;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_NONE;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

            vkCmdPipelineBarrier(
                commandBuffer->CommandBuffer,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            VkImageBlit blit = {};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = barrier.subresourceRange.aspectMask;
            blit.srcSubresource.mipLevel = level - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = layers;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = barrier.subresourceRange.aspectMask;
            blit.dstSubresource.mipLevel = level;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = layers;

            vkCmdBlitImage(commandBuffer->CommandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

            barrier.subresourceRange.baseMipLevel = level - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            vkCmdPipelineBarrier(
                commandBuffer->CommandBuffer,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        vkCmdPipelineBarrier(
            commandBuffer->CommandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
    }
}
