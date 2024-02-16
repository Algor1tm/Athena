#include "VulkanImage.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"


namespace Athena
{
	namespace Vulkan
	{
		static VkImageUsageFlags GetImageUsage(ImageUsage usage, ImageFormat format)
		{
			bool depthStencil = Image::IsDepthFormat(format) || Image::IsStencilFormat(format);

			VkImageUsageFlags flags = 0;

			if(usage & ImageUsage::SAMPLED)
				flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

			if (usage & ImageUsage::STORAGE)
				flags |= VK_IMAGE_USAGE_STORAGE_BIT;

			if(usage & ImageUsage::ATTACHMENT)
				flags |= depthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			if(usage & ImageUsage::TRANSFER_SRC)
				flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

			if (usage & ImageUsage::TRANSFER_DST)
				flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

			return (VkImageUsageFlags)flags;
		}

		static VkImageType GetImageType(ImageType type)
		{
			switch (type)
			{
			case ImageType::IMAGE_2D: return VK_IMAGE_TYPE_2D;
			case ImageType::IMAGE_CUBE: return VK_IMAGE_TYPE_2D;
			}

			ATN_CORE_ASSERT(false);
			return (VkImageType)0;
		}

		static VkImageViewType GetImageViewType(ImageType type)
		{
			switch (type)
			{
			case ImageType::IMAGE_2D: return VK_IMAGE_VIEW_TYPE_2D;
			case ImageType::IMAGE_CUBE: return VK_IMAGE_VIEW_TYPE_CUBE;
			}

			ATN_CORE_ASSERT(false);
			return (VkImageViewType)0;
		}
	}


	VulkanImage::VulkanImage(const ImageCreateInfo& info)
	{
		m_Info = info;
		m_Info.Width = 0;
		m_Info.Height = 0;
		m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;

		if (info.MipLevels == 0)
			m_Info.MipLevels = Math::Floor(Math::Log2(Math::Max((float)info.Width, (float)info.Height))) + 1;

		Resize(info.Width, info.Height);

		if (m_Info.InitialData != nullptr)
		{
			Buffer localBuffer = Buffer::Copy(m_Info.InitialData, m_Info.Width * m_Info.Height * (uint64)Image::BytesPerPixel(m_Info.Format));

			Renderer::Submit([this, localBuffer]() mutable
			{
				RT_UploadData(localBuffer.Data(), m_Info.Width, m_Info.Height);

				localBuffer.Release();
				m_Info.InitialData = nullptr;
			});

			if (m_Info.MipLevels != 1)
				BlitMipMap(m_Info.MipLevels);
		}
	}

	VulkanImage::~VulkanImage()
	{
		CleanUp();
	}

	void VulkanImage::Resize(uint32 width, uint32 height)
	{
		if (m_Info.Width == width && m_Info.Height == height)
			return;

		m_Info.Width = width;
		m_Info.Height = height;

		CleanUp();

		Renderer::Submit([this]() mutable
		{
			VkImageUsageFlags imageUsage = Vulkan::GetImageUsage(m_Info.Usage, m_Info.Format);

			if (m_Info.InitialData != nullptr)
			{
				imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}

			if (m_Info.MipLevels != 1)
			{
				imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
				imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}

			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.flags = m_Info.Type == ImageType::IMAGE_CUBE ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
			imageInfo.imageType = Vulkan::GetImageType(m_Info.Type);
			imageInfo.extent.width = m_Info.Width;
			imageInfo.extent.height = m_Info.Height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = m_Info.MipLevels;
			imageInfo.arrayLayers = m_Info.Layers;
			imageInfo.format = Vulkan::GetFormat(m_Info.Format);
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = imageUsage;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

			m_Image = VulkanContext::GetAllocator()->AllocateImage(imageInfo, VMA_MEMORY_USAGE_AUTO, VmaAllocationCreateFlagBits(0), m_Info.Name);
			Vulkan::SetObjectDebugName(m_Image.GetImage(), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, std::format("Image_{}", m_Info.Name));

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_Image.GetImage();
			viewInfo.viewType = Vulkan::GetImageViewType(m_Info.Type);
			viewInfo.format = Vulkan::GetFormat(m_Info.Format);
			viewInfo.subresourceRange.aspectMask = Vulkan::GetImageAspectMask(m_Info.Format);
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = m_Info.MipLevels;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = m_Info.Layers;

			VK_CHECK(vkCreateImageView(VulkanContext::GetLogicalDevice(), &viewInfo, nullptr, &m_ImageView));
			Vulkan::SetObjectDebugName(m_ImageView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, std::format("ImageView_{}", m_Info.Name));

			if (m_Info.MipLevels > 1)
			{
				m_ImageViewsPerMip.clear();
				m_ImageViewsPerMip.reserve(m_Info.MipLevels);
				for (uint32 mip = 0; mip < m_Info.MipLevels; ++mip)
				{
					VkImageViewCreateInfo viewInfo = {};
					viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					viewInfo.image = m_Image.GetImage();
					viewInfo.viewType = Vulkan::GetImageViewType(m_Info.Type);
					viewInfo.format = Vulkan::GetFormat(m_Info.Format);
					viewInfo.subresourceRange.aspectMask = Vulkan::GetImageAspectMask(m_Info.Format);
					viewInfo.subresourceRange.baseMipLevel = mip;
					viewInfo.subresourceRange.levelCount = 1;
					viewInfo.subresourceRange.baseArrayLayer = 0;
					viewInfo.subresourceRange.layerCount = m_Info.Layers;

					VkImageView mipView;

					VK_CHECK(vkCreateImageView(VulkanContext::GetLogicalDevice(), &viewInfo, nullptr, &mipView));
					Vulkan::SetObjectDebugName(mipView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, std::format("ImageViewMip{}_{}", mip, m_Info.Name));
					m_ImageViewsPerMip.push_back(mipView);
				}
			}

			m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
		});
	}

	void VulkanImage::CleanUp()
	{
		if (m_ImageView == VK_NULL_HANDLE && m_Image.GetImage() == VK_NULL_HANDLE)
			return;

		Renderer::SubmitResourceFree([vkImageView = m_ImageView, image = m_Image, mipViews = m_ImageViewsPerMip, name = m_Info.Name]()
		{
			for (VkImageView mipView : mipViews)
				vkDestroyImageView(VulkanContext::GetLogicalDevice(), mipView, nullptr);

			vkDestroyImageView(VulkanContext::GetLogicalDevice(), vkImageView, nullptr);
			VulkanContext::GetAllocator()->DestroyImage(image, name);
		});
	}

	void VulkanImage::RT_TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newLayout, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = m_Layout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_Image.GetImage();
		barrier.subresourceRange.aspectMask = Vulkan::GetImageAspectMask(m_Info.Format);
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = m_Info.MipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = m_Info.Layers;

		vkCmdPipelineBarrier(
			cmdBuffer,
			srcStage, dstStage,
			VK_DEPENDENCY_BY_REGION_BIT,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		m_Layout = newLayout;
	}

	void VulkanImage::RT_UploadData(const void* data, uint32 width, uint32 height)
	{
		VkDeviceSize imageSize = width * height * (uint64)Image::BytesPerPixel(m_Info.Format);

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = m_Info.Layers * imageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VulkanBufferAllocation stagingBuffer = VulkanContext::GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		void* mappedMemory = stagingBuffer.MapMemory();

		for(uint32 offset = 0; offset < m_Info.Layers * imageSize; offset += imageSize)
			memcpy((byte*)mappedMemory + offset, data, imageSize);

		stagingBuffer.UnmapMemory();

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_Image.GetImage();
		barrier.subresourceRange.aspectMask = Vulkan::GetImageAspectMask(m_Info.Format);
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = m_Info.Layers;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		VkCommandBuffer commandBuffer = Vulkan::BeginSingleTimeCommands();
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

			VkBufferImageCopy region = {};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = Vulkan::GetImageAspectMask(m_Info.Format);
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = m_Info.Layers;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { width, height, 1 };

			vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.GetBuffer(), m_Image.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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
		Vulkan::EndSingleTimeCommands(commandBuffer);
		VulkanContext::GetAllocator()->DestroyBuffer(stagingBuffer);

		m_Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	void VulkanImage::BlitMipMap(uint32 levels)
	{
		Renderer::Submit([this, levels]()
		{
			VkCommandBuffer commandBuffer = Vulkan::BeginSingleTimeCommands();
			RT_BlitMipMap(commandBuffer, levels);
			Vulkan::EndSingleTimeCommands(commandBuffer);
		});
	}

	void VulkanImage::BlitMipMap(const Ref<RenderCommandBuffer>& cmdBuffer, uint32 levels)
	{
		Renderer::Submit([this, cmdBuffer, levels]()
		{
			VkCommandBuffer commandBuffer = cmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();
			RT_BlitMipMap(commandBuffer, levels);
		});
	}

	void VulkanImage::RT_BlitMipMap(VkCommandBuffer commandBuffer, uint32 levels)
	{
		if (levels < 2 || levels > m_Info.MipLevels)
		{
			ATN_CORE_WARN_TAG("Renderer", "Attempt to generate invalid number of mip map levels (given - {}, max level - {}) of image '{}'!", levels, m_Info.MipLevels, m_Info.Name);
			ATN_CORE_ASSERT(false);
			return;
		}

		VkImage image = GetVulkanImage();

		int32 mipWidth = m_Info.Width;
		int32 mipHeight = m_Info.Height;

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = Vulkan::GetImageAspectMask(m_Info.Format);
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = m_Info.Layers;

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
			blit.srcSubresource.layerCount = m_Info.Layers;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = barrier.subresourceRange.aspectMask;
			blit.dstSubresource.mipLevel = level;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = m_Info.Layers;

			vkCmdBlitImage(commandBuffer,
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

		m_Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
}
