#include "VulkanImage.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"


namespace Athena
{
	namespace Vulkan
	{
		static VkImageUsageFlags GetImageUsage(TextureUsage usage, TextureFormat format)
		{
			bool depthStencil = Texture::IsDepthFormat(format) || Texture::IsStencilFormat(format);

			VkImageUsageFlags flags = 0;

			if(usage & TextureUsage::SAMPLED)
				flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

			if (usage & TextureUsage::STORAGE)
				flags |= VK_IMAGE_USAGE_STORAGE_BIT;

			if(usage & TextureUsage::ATTACHMENT)
				flags |= depthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			return (VkImageUsageFlags)flags;
		}
	}

	VulkanImage::VulkanImage(const TextureCreateInfo& info, TextureType type, Buffer data)
	{
		m_Info = info;
		m_Type = type;
		m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;

		if (type == TextureType::TEXTURE_CUBE)
			m_Info.Layers *= 6;

		m_Info.Width = 0;
		m_Info.Height = 0;
		Resize(info.Width, info.Height);

		if (data.Size() != 0)
		{
			UploadData(data, m_Info.Width, m_Info.Height);
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

		if (m_Info.GenerateMipMap)
			m_MipLevels = Math::Floor(Math::Log2(Math::Max<float>(m_Info.Width, m_Info.Height))) + 1;
		else
			m_MipLevels = 1;

		CleanUp();

		VkImageUsageFlags imageUsage = Vulkan::GetImageUsage(m_Info.Usage, m_Info.Format);

		imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.flags = m_Type == TextureType::TEXTURE_CUBE ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
		imageInfo.imageType = Vulkan::GetImageType(m_Type);
		imageInfo.extent.width = m_Info.Width;
		imageInfo.extent.height = m_Info.Height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = m_MipLevels;
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
		viewInfo.viewType = Vulkan::GetImageViewType(m_Type, m_Info.Layers);
		viewInfo.format = Vulkan::GetFormat(m_Info.Format);
		viewInfo.subresourceRange.aspectMask = Vulkan::GetImageAspectMask(m_Info.Format);
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = m_MipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = m_Info.Layers;

		VK_CHECK(vkCreateImageView(VulkanContext::GetLogicalDevice(), &viewInfo, nullptr, &m_ImageView));
		Vulkan::SetObjectDebugName(m_ImageView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, std::format("ImageView_{}", m_Info.Name));

		m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	void VulkanImage::CleanUp()
	{
		if (m_ImageView == VK_NULL_HANDLE && m_Image.GetImage() == VK_NULL_HANDLE)
			return;

		Renderer::SubmitResourceFree([vkImageView = m_ImageView, image = m_Image, name = m_Info.Name]()
		{
			vkDestroyImageView(VulkanContext::GetLogicalDevice(), vkImageView, nullptr);
			VulkanContext::GetAllocator()->DestroyImage(image, name);
		});
	}

	void VulkanImage::TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = m_Layout;
		barrier.newLayout = newLayout;
		barrier.srcAccessMask = srcAccess;
		barrier.dstAccessMask = dstAccess;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_Image.GetImage();
		barrier.subresourceRange.aspectMask = Vulkan::GetImageAspectMask(m_Info.Format);
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = m_MipLevels;
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

	void VulkanImage::RenderPassUpdateLayout(VkImageLayout newLayout)
	{
		m_Layout = newLayout;
	}

	void VulkanImage::UploadData(Buffer data, uint32 width, uint32 height)
	{
		ATN_CORE_ASSERT(data.Size() >= m_Info.Width * m_Info.Height * Texture::BytesPerPixel(m_Info.Format), "Buffer is too small");

		VkDeviceSize imageSize = width * height * (uint64)Texture::BytesPerPixel(m_Info.Format);

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = m_Info.Layers * imageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VulkanBufferAllocation stagingBuffer = VulkanContext::GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		void* mappedMemory = stagingBuffer.MapMemory();

		for(uint32 offset = 0; offset < m_Info.Layers * imageSize; offset += imageSize)
			memcpy((byte*)mappedMemory + offset, data.Data(), imageSize);

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
			barrier.srcAccessMask = VK_ACCESS_NONE;
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

			if (m_Info.GenerateMipMap)
			{
				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

				vkCmdPipelineBarrier(
					commandBuffer,
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					0, nullptr
				);

				Vulkan::BlitMipMap(commandBuffer, m_Image.GetImage(), m_Info.Width, m_Info.Height, m_Info.Layers, m_Info.Format, GetMipLevelsCount());
			}
			else
			{
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

		}
		Vulkan::EndSingleTimeCommands(commandBuffer);
		VulkanContext::GetAllocator()->DestroyBuffer(stagingBuffer);

		m_Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	void VulkanImage::WriteContentToBuffer(Buffer* buffer)
	{
		ATN_CORE_ASSERT(m_Type != TextureType::TEXTURE_CUBE, "Not implemented!");

		VkCommandBuffer commandBuffer = Vulkan::BeginSingleTimeCommands();
		uint32 width = m_Info.Width;
		uint32 height = m_Info.Height;
		uint32 size = m_Info.Width * m_Info.Height * Texture::BytesPerPixel(m_Info.Format);

		VkImage srcImage = GetVulkanImage();

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		VulkanBufferAllocation bufAlloc = VulkanContext::GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
		VkBuffer dstBuffer = bufAlloc.GetBuffer();

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = srcImage;
		barrier.subresourceRange.aspectMask = Vulkan::GetImageAspectMask(m_Info.Format);
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = m_Info.Layers;

		{
			barrier.oldLayout = m_Layout;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_NONE;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}

		VkBufferImageCopy copy = {};
		copy.bufferOffset = 0;
		copy.bufferRowLength = width;
		copy.bufferImageHeight = height;
		copy.imageSubresource.aspectMask = Vulkan::GetImageAspectMask(m_Info.Format);
		copy.imageSubresource.mipLevel = 0;
		copy.imageSubresource.baseArrayLayer = 0;
		copy.imageSubresource.layerCount = 1;
		copy.imageOffset = { 0, 0, 0 };
		copy.imageExtent = { width, height, 1 };

		vkCmdCopyImageToBuffer(commandBuffer, 
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
			dstBuffer, 
			1, &copy);

		{
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_NONE;

			VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

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

		buffer->Allocate(size);

		void* memory = bufAlloc.MapMemory();
		buffer->Write(memory, size);
		bufAlloc.UnmapMemory();

		VulkanContext::GetAllocator()->DestroyBuffer(bufAlloc);
	}
}
