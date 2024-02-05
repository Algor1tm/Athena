#include "VulkanImage.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	namespace Vulkan
	{
		static VkImageUsageFlags GetImageUsage(ImageUsage usage, ImageFormat format)
		{
			bool depthStencil = Image::IsDepthFormat(format) || Image::IsStencilFormat(format);

			switch (usage)
			{
			case ImageUsage::SHADER_READ_ONLY: return VK_IMAGE_USAGE_SAMPLED_BIT;
			case ImageUsage::ATTACHMENT: return depthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			}

			ATN_CORE_ASSERT(false);
			return (VkImageUsageFlags)0;
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

		if (info.MipLevels == 0)
			m_Info.MipLevels = Math::Floor(Math::Log2(Math::Max((float)info.Width, (float)info.Height))) + 1;

		Renderer::Submit([this]() mutable
		{
			uint32 imageUsage = Vulkan::GetImageUsage(m_Info.Usage, m_Info.Format);

			if (m_Info.Usage == ImageUsage::ATTACHMENT)
			{
				imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
				imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;	// TODO (this usage added for copying into swapchain)
			}

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
			Vulkan::SetObjectName(m_Image.GetImage(), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, std::format("Image_{}", m_Info.Name));

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
			Vulkan::SetObjectName(m_ImageView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, std::format("ImageView_{}", m_Info.Name));
		});

		if (m_Info.InitialData != nullptr)
		{
			Buffer localBuffer = Buffer::Copy(m_Info.InitialData, m_Info.Width * m_Info.Height * (uint64)Image::BytesPerPixel(m_Info.Format));

			Renderer::Submit([this, localBuffer]() mutable
			{
				RT_UploadData(localBuffer.Data(), m_Info.Width, m_Info.Height);

				localBuffer.Release();
				m_Info.InitialData = nullptr;
			});
		}
	}

	VulkanImage::~VulkanImage()
	{
		Renderer::SubmitResourceFree([vkImageView = m_ImageView, image = m_Image, name = m_Info.Name]()
		{
			vkDestroyImageView(VulkanContext::GetLogicalDevice(), vkImageView, nullptr);
			VulkanContext::GetAllocator()->DestroyImage(image, name);
		});
	}

	void VulkanImage::RT_UploadData(const void* data, uint32 width, uint32 height)
	{
		VkDeviceSize imageSize = width * height * (uint64)Image::BytesPerPixel(m_Info.Format);

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = imageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VulkanBufferAllocation stagingBuffer = VulkanContext::GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		void* mappedMemory = stagingBuffer.MapMemory();
		memcpy(mappedMemory, data, imageSize);
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
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		VkCommandBuffer commandBuffer = Vulkan::BeginSingleTimeCommands();
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

			region.imageSubresource.aspectMask = Vulkan::GetImageAspectMask(m_Info.Format);
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
		Vulkan::EndSingleTimeCommands(commandBuffer);
		VulkanContext::GetAllocator()->DestroyBuffer(stagingBuffer);
	}
}
