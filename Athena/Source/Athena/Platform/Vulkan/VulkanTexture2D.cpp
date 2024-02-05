#include "VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"


namespace Athena
{
	VulkanTexture2D::VulkanTexture2D(const TextureCreateInfo& info)
	{
		m_Info = info;

		m_DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_DescriptorInfo.imageView = VK_NULL_HANDLE;
		m_DescriptorInfo.sampler = VK_NULL_HANDLE;

		SetSampler(m_Info.SamplerInfo);
		Resize(m_Info.Width, m_Info.Height);
	}

	VulkanTexture2D::VulkanTexture2D(const Ref<Image>& image, const TextureSamplerCreateInfo& samplerInfo)
	{
		const ImageCreateInfo& imageInfo = image->GetInfo();

		m_Info.Name = imageInfo.Name;
		m_Info.Format = imageInfo.Format;
		m_Info.Usage = imageInfo.Usage;
		m_Info.InitialData = imageInfo.InitialData;
		m_Info.Width = imageInfo.Width;
		m_Info.Height = imageInfo.Height;
		m_Info.Layers = imageInfo.Layers;
		m_Info.MipLevels = imageInfo.MipLevels;
		m_Info.SamplerInfo = samplerInfo;

		m_Image = image;

		m_DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_DescriptorInfo.imageView = VK_NULL_HANDLE;
		m_DescriptorInfo.sampler = VK_NULL_HANDLE;

		SetSampler(m_Info.SamplerInfo);

		if (m_Info.MipLevels != 1)
			GenerateMipMap(m_Info.MipLevels);
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		Renderer::SubmitResourceFree([vkSampler = m_Sampler]()
		{
			vkDestroySampler(VulkanContext::GetLogicalDevice(), vkSampler, nullptr);
		});
	}

	void VulkanTexture2D::Resize(uint32 width, uint32 height)
	{
		m_Info.Width = width;
		m_Info.Height = height;

		ImageCreateInfo imageInfo;
		imageInfo.Name = m_Info.Name;
		imageInfo.Format = m_Info.Format;
		imageInfo.Usage = m_Info.Usage;
		imageInfo.Type = ImageType::IMAGE_2D;
		imageInfo.InitialData = m_Info.InitialData;
		imageInfo.Width = m_Info.Width;
		imageInfo.Height = m_Info.Height;
		imageInfo.Layers = m_Info.Layers;
		imageInfo.MipLevels = m_Info.MipLevels;

		m_Image = Image::Create(imageInfo);

		if (m_Info.MipLevels != 1)
			GenerateMipMap(m_Info.MipLevels);
	}

	void VulkanTexture2D::SetSampler(const TextureSamplerCreateInfo& samplerInfo)
	{
		if (m_Sampler != VK_NULL_HANDLE)
		{
			Renderer::SubmitResourceFree([vkSampler = m_Sampler]()
			{
				vkDestroySampler(VulkanContext::GetLogicalDevice(), vkSampler, nullptr);
			});
		}

		m_Info.SamplerInfo = samplerInfo;

		Renderer::Submit([this]()
		{
			VkSamplerCreateInfo vksamplerInfo = {};
			vksamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			vksamplerInfo.magFilter = Vulkan::GetFilter(m_Info.SamplerInfo.MagFilter);
			vksamplerInfo.minFilter = Vulkan::GetFilter(m_Info.SamplerInfo.MinFilter);
			vksamplerInfo.mipmapMode = Vulkan::GetMipMapMode(m_Info.SamplerInfo.MipMapFilter);
			vksamplerInfo.addressModeU = Vulkan::GetWrap(m_Info.SamplerInfo.Wrap);
			vksamplerInfo.addressModeV = Vulkan::GetWrap(m_Info.SamplerInfo.Wrap);
			vksamplerInfo.addressModeW = Vulkan::GetWrap(m_Info.SamplerInfo.Wrap);
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
			Vulkan::SetObjectName(m_Sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, std::format("Sampler_{}", m_Info.Name));

			m_DescriptorInfo.sampler = m_Sampler;
		});
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

			VkImage image = GetVulkanImage();
			VkCommandBuffer commandBuffer = Vulkan::BeginSingleTimeCommands();

			int32 mipWidth = m_Info.Width;
			int32 mipHeight = m_Info.Height;

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = Vulkan::GetImageAspectMask(m_Info.Format);;
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

			Vulkan::EndSingleTimeCommands(commandBuffer);
		});
	}

	VkImage VulkanTexture2D::GetVulkanImage() const 
	{
		return m_Image.As<VulkanImage>()->GetVulkanImage();
	}

	VkImageView VulkanTexture2D::GetVulkanImageView() const
	{ 
		return m_Image.As<VulkanImage>()->GetVulkanImageView();
	}

	const VkDescriptorImageInfo& VulkanTexture2D::GetVulkanDescriptorInfo()
	{
		m_DescriptorInfo.imageView = GetVulkanImageView();
		return m_DescriptorInfo;
	}
}
