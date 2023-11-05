#include "VulkanTexture2D.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"

#include <ImGui/backends/imgui_impl_vulkan.h>


namespace Athena
{
	static VkImageUsageFlags GetVulkanImageUsage(TextureUsage usage, TextureFormat format)
	{
		bool depthStencil = Texture::IsDepthFormat(format) || Texture::IsStencilFormat(format);

		switch (usage)
		{
		case TextureUsage::SHADER_READ_ONLY: return VK_IMAGE_USAGE_SAMPLED_BIT;
		case TextureUsage::ATTACHMENT: return depthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		ATN_CORE_ASSERT(false);
		return (VkImageUsageFlags)0;
	}


	VulkanTexture2D::VulkanTexture2D(const TextureCreateInfo& info)
	{
		m_Info = info;

		Buffer localBuffer;
		if(info.Data != nullptr)
			localBuffer = Buffer::Copy(info.Data, info.Width * info.Height * (uint64)Texture::BytesPerPixel(info.Format));

		if (info.GenerateMipMap && info.MipLevels == 0)
			m_Info.MipLevels = Math::Floor(Math::Log2(Math::Max((float)info.Width, (float)info.Height))) + 1;

		Ref<VulkanTexture2D> instance(this);
		Renderer::Submit([instance, localBuffer]() mutable
		{
			auto& info = instance->m_Info;

			uint32 imageUsage = GetVulkanImageUsage(info.Usage, info.Format);

			if (info.Usage == TextureUsage::ATTACHMENT)
			{
				imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
				imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;	// TODO (this usage added for copying into swapchain)
			}

			if (info.Data != nullptr)
			{
				imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}

			if (info.GenerateMipMap)
			{
				imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
				imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}

			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = info.Width;
			imageInfo.extent.height = info.Height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = info.MipLevels;
			imageInfo.arrayLayers = info.Layers;
			imageInfo.format = VulkanUtils::GetFormat(info.Format, info.sRGB);
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = imageUsage;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

			instance->m_Image = VulkanContext::GetAllocator()->AllocateImage(imageInfo);

			if (info.Data != nullptr)
			{
				instance->UploadData(localBuffer.Data(), info.Width, info.Height);

				localBuffer.Release();
				info.Data = nullptr;
			}

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = instance->m_Image.GetImage();
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = VulkanUtils::GetFormat(info.Format, info.sRGB);
			viewInfo.subresourceRange.aspectMask = VulkanUtils::GetImageAspectMask(info.Format);
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = info.MipLevels;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = info.Layers;

			VK_CHECK(vkCreateImageView(VulkanContext::GetLogicalDevice(), &viewInfo, nullptr, &instance->m_ImageView));
		});

		if(m_Info.GenerateSampler)
			SetSampler(m_Info.SamplerInfo);

		if (m_Info.GenerateMipMap)
			GenerateMipMap(m_Info.MipLevels);
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		Renderer::SubmitResourceFree([set = m_UIDescriptorSet, vkSampler = m_Sampler, vkImageView = m_ImageView,
				image = m_Image]()
		{
			// TODO: some imgui descriptor sets does not removed on application close,
			// because they deleted after imgui layer shutdowned

			if(set != VK_NULL_HANDLE && Application::Get().GetImGuiLayer() != nullptr) 
				ImGui_ImplVulkan_RemoveTexture(set);

			vkDestroySampler(VulkanContext::GetLogicalDevice(), vkSampler, nullptr);
			vkDestroyImageView(VulkanContext::GetLogicalDevice(), vkImageView, nullptr);

			VulkanContext::GetAllocator()->DestroyImage(image);
		});
	}

	void VulkanTexture2D::SetSampler(const TextureSamplerCreateInfo& samplerInfo)
	{
		if (m_Sampler != VK_NULL_HANDLE)
		{
			Renderer::SubmitResourceFree([vkSampler = m_Sampler, set = m_UIDescriptorSet]()
			{
				vkDestroySampler(VulkanContext::GetLogicalDevice(), vkSampler, nullptr);

				if(set != VK_NULL_HANDLE)
					ImGui_ImplVulkan_RemoveTexture(set);
			});
		}

		m_Info.SamplerInfo = samplerInfo;

		Ref<VulkanTexture2D> instance(this);
		Renderer::Submit([instance]()
		{
			auto& info = instance->m_Info;

			VkSamplerCreateInfo vksamplerInfo = {};
			vksamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			vksamplerInfo.magFilter = VulkanUtils::GetFilter(info.SamplerInfo.MagFilter);
			vksamplerInfo.minFilter = VulkanUtils::GetFilter(info.SamplerInfo.MinFilter);
			vksamplerInfo.mipmapMode = VulkanUtils::GetMipMapMode(info.SamplerInfo.MipMapFilter);
			vksamplerInfo.addressModeU = VulkanUtils::GetWrap(info.SamplerInfo.Wrap);
			vksamplerInfo.addressModeV = VulkanUtils::GetWrap(info.SamplerInfo.Wrap);
			vksamplerInfo.addressModeW = VulkanUtils::GetWrap(info.SamplerInfo.Wrap);
			vksamplerInfo.anisotropyEnable = false;
			vksamplerInfo.maxAnisotropy = 1.f;
			vksamplerInfo.compareEnable = false;
			vksamplerInfo.compareOp = VK_COMPARE_OP_NEVER;
			vksamplerInfo.minLod = 0.f;
			vksamplerInfo.maxLod = info.MipLevels;
			vksamplerInfo.mipLodBias = 0.f;
			vksamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			vksamplerInfo.unnormalizedCoordinates = false;

			VK_CHECK(vkCreateSampler(VulkanContext::GetLogicalDevice(), &vksamplerInfo, nullptr, &instance->m_Sampler));
		});
	}

	void VulkanTexture2D::UploadData(const void* data, uint32 width, uint32 height)
	{
		VkDeviceSize imageSize = width * height * (uint64)Texture::BytesPerPixel(m_Info.Format);

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = imageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VulkanBuffer stagingBuffer = VulkanContext::GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		void* mappedMemory = stagingBuffer.MapMemory();
		memcpy(mappedMemory, data, imageSize);
		stagingBuffer.UnmapMemory();

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_Image.GetImage();
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands();
		{
			// Barrier
			{
				barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

				vkCmdPipelineBarrier(
					commandBuffer,
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);
			}

			VkBufferImageCopy region = {};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VulkanUtils::GetImageAspectMask(m_Info.Format);
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { width, height, 1 };

			vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.GetBuffer(), m_Image.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			// Barrier
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}
		VulkanUtils::EndSingleTimeCommands(commandBuffer);

		VulkanContext::GetAllocator()->DestroyBuffer(stagingBuffer);
	}

	void VulkanTexture2D::GenerateMipMap(uint32 levels)
	{
		Ref<VulkanTexture2D> instance(this);
		Renderer::Submit([instance, levels]()
		{
			instance->m_Info.MipLevels = levels;

			if (instance->m_Info.MipLevels < 2)
			{
				ATN_CORE_ASSERT(false);
				return;
			}

			VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands();

			int32 mipWidth = instance->m_Info.Width;
			int32 mipHeight = instance->m_Info.Height;

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = instance->m_Image.GetImage();
			barrier.subresourceRange.aspectMask = VulkanUtils::GetImageAspectMask(instance->m_Info.Format);;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			for (uint32 level = 1; level < instance->m_Info.MipLevels; ++level)
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
					commandBuffer,
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
					commandBuffer,
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
				blit.srcSubresource.layerCount = 1;
				blit.dstOffsets[0] = { 0, 0, 0 };
				blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
				blit.dstSubresource.aspectMask = barrier.subresourceRange.aspectMask;
				blit.dstSubresource.mipLevel = level;
				blit.dstSubresource.baseArrayLayer = 0;
				blit.dstSubresource.layerCount = 1;

				vkCmdBlitImage(commandBuffer,
					instance->m_Image.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					instance->m_Image.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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
					commandBuffer,
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;
			}

			barrier.subresourceRange.baseMipLevel = instance->m_Info.MipLevels - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VulkanUtils::EndSingleTimeCommands(commandBuffer);
		});

	}

	void* VulkanTexture2D::GetDescriptorSet()
	{
		if (m_UIDescriptorSet == VK_NULL_HANDLE)
		{
			ATN_CORE_ASSERT(m_Sampler);
			m_UIDescriptorSet = ImGui_ImplVulkan_AddTexture(m_Sampler, m_ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		return m_UIDescriptorSet;
	}
}
