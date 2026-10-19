#include "VulkanTexture.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"


namespace Athena
{
	namespace Vulkan
	{
		static VkImageUsageFlags TranslateImageUsage(TextureUsage usage, Format format)
		{
			auto& formatProperties = GetFormatProperties(format);
			bool depthStencil = formatProperties.HasDepth || formatProperties.HasStencil;

			VkImageUsageFlags flags = 0;

			if (usage & TextureUsage::SAMPLED)
				flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

			if (usage & TextureUsage::STORAGE)
				flags |= VK_IMAGE_USAGE_STORAGE_BIT;

			if (usage & TextureUsage::ATTACHMENT)
				flags |= depthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			return (VkImageUsageFlags)flags;
		}

		inline VkImageType TranslateTextureType(TextureType type)
		{
			switch (type)
			{
			case TextureType::TEXTURE_2D:   return VK_IMAGE_TYPE_2D;
			case TextureType::TEXTURE_CUBE: return VK_IMAGE_TYPE_2D;
			}

			checkf(false);
			return (VkImageType)0;
		}

		inline VkSampleCountFlagBits TranslateSamplesCount(uint32 samples)
		{
			switch (samples)
			{
			case 1: return VK_SAMPLE_COUNT_1_BIT;
			case 2: return VK_SAMPLE_COUNT_2_BIT;
			case 4: return VK_SAMPLE_COUNT_4_BIT;
			case 8: return VK_SAMPLE_COUNT_8_BIT;
			case 16: return VK_SAMPLE_COUNT_16_BIT;
			}

			checkf(false);
			return VK_SAMPLE_COUNT_1_BIT;
		}
	}

	VulkanTexture::VulkanTexture(const TextureCreateInfo& info)
		: Texture(info)
	{

	}

	VulkanTexture::~VulkanTexture()
	{
		CleanUp();
	}

	void VulkanTexture::Init()
	{
		TRACY_PROFILE_FUNC();

		Resize(GetWidth(), GetHeight());
	}

	void VulkanTexture::Resize(uint32 width, uint32 height)
	{
		TRACY_PROFILE_FUNC();

		Texture::Resize(width, height);

		CleanUp();

		VkImageUsageFlags imageUsage = Vulkan::TranslateImageUsage(GetInfo().Usage, GetInfo().TextureFormat);

		imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.flags = GetType() == TextureType::TEXTURE_CUBE ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
		imageInfo.imageType = Vulkan::TranslateTextureType(GetType());
		imageInfo.extent.width = GetWidth();
		imageInfo.extent.height = GetHeight();
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = GetMipLevelsCount();
		imageInfo.arrayLayers = GetInfo().Layers;
		imageInfo.format = Vulkan::TranslateFormat(GetInfo().TextureFormat);
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = imageUsage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = Vulkan::TranslateSamplesCount(GetInfo().Samples);

		m_Image = VulkanRHI::Get()->GetAllocator()->AllocateImage(imageInfo, VMA_MEMORY_USAGE_AUTO, 0, GetName());
		VK_SET_OBJECT_NAME(m_Image.GetImage(), VK_OBJECT_TYPE_IMAGE, fmt::format("Image_{}", GetName()));

		InvalidateViews();
	}

	void VulkanTexture::Upload(const Buffer& srcBuffer, const TextureRegion& region)
	{
		ensuref(false);
	}

	void VulkanTexture::UploadAndBlitMipMap(const Buffer& srcBuffer)
	{
		TRACY_PROFILE_FUNC();

		VkDeviceSize imageSize = GetWidth() * GetHeight() * GetFormatProperties(GetFormat()).BytesPerPixel;

		ensure(srcBuffer.Size() <= imageSize, "Texture upload buffer is too small!");

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = GetInfo().Layers * imageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VulkanBufferAllocation stagingBuffer = VulkanRHI::Get()->GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		void* mappedMemory = stagingBuffer.MapMemory();

		for (uint32 offset = 0; offset < GetInfo().Layers * imageSize; offset += imageSize)
			memcpy((byte*)mappedMemory + offset, srcBuffer.Data(), imageSize);

		stagingBuffer.UnmapMemory();

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_Image.GetImage();
		barrier.subresourceRange.aspectMask = Vulkan::TranslateTextureAspect(TextureAspect::AUTO, GetFormat());
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = GetInfo().Layers;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		VulkanCommandBuffer* commandBuffer = VulkanRHI::Get()->GetCmdBufferManager()->GetTransferCommandBuffer();
		{
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_NONE;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			vkCmdPipelineBarrier(
				commandBuffer->CommandBuffer,
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

			region.imageSubresource.aspectMask = barrier.subresourceRange.aspectMask;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = GetInfo().Layers;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { GetWidth(), GetHeight(), 1};

			vkCmdCopyBufferToImage(commandBuffer->CommandBuffer, stagingBuffer.GetBuffer(), m_Image.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			if (GetInfo().GenerateMipMap)
			{
				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

				vkCmdPipelineBarrier(
					commandBuffer->CommandBuffer,
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					0, nullptr
				);

				Vulkan::BlitMipMap(commandBuffer, m_Image.GetImage(), GetWidth(), GetHeight(), GetInfo().Layers, GetFormat(), GetMipLevelsCount());
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
					commandBuffer->CommandBuffer,
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);
			}

		}

		commandBuffer->AddCallbackOnCompletion([stagingBuffer]() 
			{
				VulkanRHI::Get()->GetAllocator()->DestroyBuffer(stagingBuffer);
			});

	}

	void VulkanTexture::CleanUp()
	{
		if (m_Image)
		{
			VulkanRHI::Get()->DeleteFrameResourcesDeferred([image = m_Image, name = GetName()]()
			{
				VulkanRHI::Get()->GetAllocator()->DestroyImage(image, name);
			});
		}
	}
}