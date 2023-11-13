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

		if (info.GenerateMipMap && info.MipLevels <= 1)
			m_Info.MipLevels = Math::Floor(Math::Log2(Math::Max((float)info.Width, (float)info.Height))) + 1;

		if(m_Info.GenerateSampler)
			SetSampler(m_Info.SamplerInfo);

		Resize(m_Info.Width, m_Info.Height);
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

	void VulkanTexture2D::Resize(uint32 width, uint32 height)
	{
		if (m_ImageView != VK_NULL_HANDLE)
		{
			Renderer::SubmitResourceFree([set = m_UIDescriptorSet, vkImageView = m_ImageView, image = m_Image]()
			{
				if (set != VK_NULL_HANDLE && Application::Get().GetImGuiLayer() != nullptr)
					ImGui_ImplVulkan_RemoveTexture(set);

				vkDestroyImageView(VulkanContext::GetLogicalDevice(), vkImageView, nullptr);
				VulkanContext::GetAllocator()->DestroyImage(image);
			});
		}

		m_Info.Width = width;
		m_Info.Height = height;

		Renderer::Submit([this]() mutable
		{
			uint32 imageUsage = GetVulkanImageUsage(m_Info.Usage, m_Info.Format);

			if (m_Info.Usage == TextureUsage::ATTACHMENT)
			{
				imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
				imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;	// TODO (this usage added for copying into swapchain)
			}

			if (m_Info.Data != nullptr)
			{
				imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}

			if (m_Info.GenerateMipMap)
			{
				imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
				imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}

			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = m_Info.Width;
			imageInfo.extent.height = m_Info.Height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = m_Info.MipLevels;
			imageInfo.arrayLayers = m_Info.Layers;
			imageInfo.format = VulkanUtils::GetFormat(m_Info.Format);
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = imageUsage;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

			m_Image = VulkanContext::GetAllocator()->AllocateImage(imageInfo);

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_Image.GetImage();
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = VulkanUtils::GetFormat(m_Info.Format);
			viewInfo.subresourceRange.aspectMask = VulkanUtils::GetImageAspectMask(m_Info.Format);
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = m_Info.MipLevels;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = m_Info.Layers;

			VK_CHECK(vkCreateImageView(VulkanContext::GetLogicalDevice(), &viewInfo, nullptr, &m_ImageView));

			m_UIDescriptorSet = VK_NULL_HANDLE;
		});


		if (m_Info.Data != nullptr)
		{
			Buffer localBuffer = Buffer::Copy(m_Info.Data, m_Info.Width * m_Info.Height * (uint64)Texture::BytesPerPixel(m_Info.Format));

			Renderer::Submit([this, localBuffer]() mutable
			{
				UploadData(localBuffer.Data(), m_Info.Width, m_Info.Height);

				localBuffer.Release();
				m_Info.Data = nullptr;
			});
		}

		if (m_Info.GenerateMipMap)
			GenerateMipMap(m_Info.MipLevels);
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

		Renderer::Submit([this]()
		{
			VkSamplerCreateInfo vksamplerInfo = {};
			vksamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			vksamplerInfo.magFilter = VulkanUtils::GetFilter(m_Info.SamplerInfo.MagFilter);
			vksamplerInfo.minFilter = VulkanUtils::GetFilter(m_Info.SamplerInfo.MinFilter);
			vksamplerInfo.mipmapMode = VulkanUtils::GetMipMapMode(m_Info.SamplerInfo.MipMapFilter);
			vksamplerInfo.addressModeU = VulkanUtils::GetWrap(m_Info.SamplerInfo.Wrap);
			vksamplerInfo.addressModeV = VulkanUtils::GetWrap(m_Info.SamplerInfo.Wrap);
			vksamplerInfo.addressModeW = VulkanUtils::GetWrap(m_Info.SamplerInfo.Wrap);
			vksamplerInfo.anisotropyEnable = false;
			vksamplerInfo.maxAnisotropy = 1.f;
			vksamplerInfo.compareEnable = false;
			vksamplerInfo.compareOp = VK_COMPARE_OP_NEVER;
			vksamplerInfo.minLod = 0.f;
			vksamplerInfo.maxLod = m_Info.MipLevels;
			vksamplerInfo.mipLodBias = 0.f;
			vksamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			vksamplerInfo.unnormalizedCoordinates = false;

			VK_CHECK(vkCreateSampler(VulkanContext::GetLogicalDevice(), &vksamplerInfo, nullptr, &m_Sampler));
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
		Renderer::Submit([this, levels]()
		{
			m_Info.MipLevels = levels;

			if (m_Info.MipLevels < 2)
			{
				ATN_CORE_ASSERT(false);
				return;
			}

			VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands();

			int32 mipWidth = m_Info.Width;
			int32 mipHeight = m_Info.Height;

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_Image.GetImage();
			barrier.subresourceRange.aspectMask = VulkanUtils::GetImageAspectMask(m_Info.Format);;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			for (uint32 level = 1; level < m_Info.MipLevels; ++level)
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
					m_Image.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					m_Image.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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

			barrier.subresourceRange.baseMipLevel = m_Info.MipLevels - 1;
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
