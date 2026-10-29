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

			if (usage & TextureUsage::UAV)
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

		static uint32 GetLayerMultiplier(TextureType type)
		{
			if (type == TextureType::TEXTURE_CUBE)
				return 6;

			return 1;
		}

		static const char* ViewTypeToString(TextureViewType type)
		{
			switch (type)
			{
			case TextureViewType::SRV: return "SRV";
			case TextureViewType::UAV: return "UAV";
			case TextureViewType::RTV: return "RTV";
			case TextureViewType::DSV: return "DSV";
			case TextureViewType::DSV_READONLY: return "DSV";
			}

			checkf(false);
			return "Unknown";
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
		imageInfo.arrayLayers = GetInfo().Layers * Vulkan::GetLayerMultiplier(GetType());
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
		bufferInfo.size = GetInfo().Layers * Vulkan::GetLayerMultiplier(GetType()) * imageSize;
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
		barrier.subresourceRange.layerCount = GetInfo().Layers * Vulkan::GetLayerMultiplier(GetType());

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
			region.imageSubresource.layerCount = GetInfo().Layers * Vulkan::GetLayerMultiplier(GetType());

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

				Vulkan::BlitMipMap(commandBuffer, m_Image.GetImage(), GetWidth(), GetHeight(), GetInfo().Layers * Vulkan::GetLayerMultiplier(GetType()), GetFormat(), GetMipLevelsCount());
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


	VulkanTextureView::VulkanTextureView(const TextureViewCreateInfo & info, Texture * texture)
		: TextureView(info, texture)
	{
		TRACY_PROFILE_FUNC();

		if (GetInfo().Name.empty())
		{
			const auto& info = GetInfo();

			m_Name = fmt::format("{}_{}_Mip[{},{}]_Layer[{},{}]", GetTexture()->GetName(), Vulkan::ViewTypeToString(GetViewType()),
				info.Range.BaseMipLevel, info.Range.BaseMipLevel + info.Range.MipLevelCount - 1,
				info.Range.BaseLayer, info.Range.BaseLayer + info.Range.LayerCount - 1);
		}
		else
		{
			m_Name = GetInfo().Name;
		}

		if (!GetInfo().OverrideSampler)
		{
			SetSampler(GetTexture()->GetInfo().Sampler);
		}
	}

	VulkanTextureView::~VulkanTextureView()
	{
		CleanUp();

		if (m_Sampler)
		{
			VulkanRHI::Get()->DeleteFrameResourcesDeferred([this, samplerInfo = GetInfo().Sampler, vkSampler = m_Sampler]()
				{
					VulkanRHI::Get()->GetAllocator()->DestroySampler(samplerInfo, vkSampler);
				});
		}
	}

	void VulkanTextureView::Init()
	{
		TRACY_PROFILE_FUNC();

		m_Sampler = VulkanRHI::Get()->GetAllocator()->CreateSampler(GetInfo().Sampler);
		m_DescriptorInfo.sampler = m_Sampler;

		Invalidate();
	}

	void VulkanTextureView::Invalidate()
	{
		TRACY_PROFILE_FUNC();

		if (m_ImageView)
		{
			CleanUp();
		}

		if (GetTexture() == nullptr)
		{
			checkf(false);
			return;
		}

		Format format = GetTexture()->GetFormat();

		VkComponentMapping swizzling = {};
		swizzling.r = Vulkan::TranslateSwizzle(GetInfo().Swizzle.R);
		swizzling.g = Vulkan::TranslateSwizzle(GetInfo().Swizzle.G);
		swizzling.b = Vulkan::TranslateSwizzle(GetInfo().Swizzle.B);
		swizzling.a = Vulkan::TranslateSwizzle(GetInfo().Swizzle.A);

		uint32 texMipLevelsCount = GetTexture()->GetMipLevelsCount();
		uint32 viewBaseMipLevel = GetRange().BaseMipLevel;
		uint32 viewMipLevelCount = GetRange().MipLevelCount;

		if (viewBaseMipLevel >= texMipLevelsCount)
			viewBaseMipLevel = texMipLevelsCount - 1;

		if (viewBaseMipLevel + viewMipLevelCount > texMipLevelsCount)
			viewMipLevelCount = texMipLevelsCount - viewBaseMipLevel;

		uint32 layersCount = GetRange().LayerCount * Vulkan::GetLayerMultiplier(GetTextureType());;

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = GetVulkanTexture()->GetImage();
		viewInfo.viewType = Vulkan::TranslateImageViewType(GetTextureType(), layersCount);
		viewInfo.format = Vulkan::TranslateFormat(format);
		viewInfo.subresourceRange.aspectMask = Vulkan::TranslateTextureAspect(GetRange().Aspect, format);
		viewInfo.subresourceRange.baseMipLevel = viewBaseMipLevel;
		viewInfo.subresourceRange.levelCount = viewMipLevelCount;
		viewInfo.subresourceRange.baseArrayLayer = GetRange().BaseLayer * Vulkan::GetLayerMultiplier(GetTextureType());
		viewInfo.subresourceRange.layerCount = layersCount;
		viewInfo.components = swizzling;

		VK_CHECK(vkCreateImageView(Vulkan::GetLogicalDevice(), &viewInfo, nullptr, &m_ImageView));
		VK_SET_OBJECT_NAME(m_ImageView, VK_OBJECT_TYPE_IMAGE_VIEW, m_Name);
	}

	void VulkanTextureView::CleanUp()
	{
		if (m_ImageView)
		{
			VulkanRHI::Get()->DeleteFrameResourcesDeferred([this, imageView = m_ImageView]()
				{
					vkDestroyImageView(Vulkan::GetLogicalDevice(), imageView, nullptr);
				});
		}
	}

	VulkanTexture* VulkanTextureView::GetVulkanTexture() const
	{
		return reinterpret_cast<VulkanTexture*>(GetTexture());
	}

	const VkDescriptorImageInfo& VulkanTextureView::GetVulkanDescriptorInfo()
	{
		m_DescriptorInfo.imageView = m_ImageView;
		m_DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		// Set default layout if image has not initalized yet
		if (m_DescriptorInfo.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED)
			m_DescriptorInfo.imageLayout = GetTexture()->GetInfo().Usage & TextureUsage::UAV ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		return m_DescriptorInfo;
	}
}