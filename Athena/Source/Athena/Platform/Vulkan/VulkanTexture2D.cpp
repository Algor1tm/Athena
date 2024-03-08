#include "VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"


namespace Athena
{
	VulkanTexture2D::VulkanTexture2D(const Texture2DCreateInfo& info)
	{
		m_Info = info;
		m_Sampler = VK_NULL_HANDLE;
		m_DescriptorInfo.sampler = m_Sampler;

		ImageCreateInfo imageInfo;
		imageInfo.Name = m_Info.Name;
		imageInfo.Format = m_Info.Format;
		imageInfo.Usage = m_Info.Usage;
		imageInfo.Type = ImageType::IMAGE_2D;
		imageInfo.InitialData = m_Info.InitialData;
		imageInfo.Width = m_Info.Width;
		imageInfo.Height = m_Info.Height;
		imageInfo.Layers = m_Info.Layers;
		imageInfo.GenerateMipLevels = m_Info.GenerateMipLevels;

		m_Image = Image::Create(imageInfo);

		SetSampler(m_Info.SamplerInfo);
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
		m_Info.GenerateMipLevels = imageInfo.GenerateMipLevels;
		m_Info.SamplerInfo = samplerInfo;

		m_Image = image;

		m_Sampler = VK_NULL_HANDLE;

		m_DescriptorInfo.imageLayout = m_Info.Usage & ImageUsage::STORAGE ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_DescriptorInfo.imageView = VK_NULL_HANDLE;
		m_DescriptorInfo.sampler = m_Sampler;

		SetSampler(m_Info.SamplerInfo);
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		Renderer::SubmitResourceFree([samplerInfo = m_Info.SamplerInfo, vkSampler = m_Sampler]()
		{
			VulkanContext::GetAllocator()->DestroySampler(samplerInfo, vkSampler);
		});
	}

	void VulkanTexture2D::Resize(uint32 width, uint32 height)
	{
		if (m_Info.Width == width && m_Info.Height == height)
			return;

		m_Info.Width = width;
		m_Info.Height = height;

		m_Image->Resize(width, height);
	}

	void VulkanTexture2D::SetSampler(const TextureSamplerCreateInfo& samplerInfo)
	{
		if (m_Sampler != VK_NULL_HANDLE)
		{
			Renderer::SubmitResourceFree([this]()
			{
				VulkanContext::GetAllocator()->DestroySampler(m_Info.SamplerInfo, m_Sampler);
			});
		}

		m_Info.SamplerInfo = samplerInfo;

		Renderer::Submit([this]()
		{
			m_Sampler = VulkanContext::GetAllocator()->AllocateSampler(m_Info.SamplerInfo);
			m_DescriptorInfo.sampler = m_Sampler;
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

	const VkDescriptorImageInfo& VulkanTexture2D::GetVulkanDescriptorInfo(uint32 mip)
	{
		m_DescriptorInfo.imageView = GetVulkanImageView();
		m_DescriptorInfo.imageLayout = m_Image.As<VulkanImage>()->GetLayout();

		if (mip != 0)
			m_DescriptorInfo.imageView = m_Image.As<VulkanImage>()->GetVulkanImageViewMip(mip);

		// Set default layout if image has not initalized yet
		if (m_DescriptorInfo.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED)
			m_DescriptorInfo.imageLayout = m_Info.Usage & ImageUsage::STORAGE ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		return m_DescriptorInfo;
	}
}
