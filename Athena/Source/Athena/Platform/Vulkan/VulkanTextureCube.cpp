#include "VulkanTextureCube.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"


namespace Athena
{
	VulkanTextureCube::VulkanTextureCube(const TextureCubeCreateInfo& info)
	{
		m_Info = info;
		m_Sampler = VK_NULL_HANDLE;
		m_DescriptorInfo.sampler = m_Sampler;

		ImageCreateInfo imageInfo;
		imageInfo.Name = m_Info.Name;
		imageInfo.Format = m_Info.Format;
		imageInfo.Usage = m_Info.Usage;
		imageInfo.Type = ImageType::IMAGE_CUBE;
		imageInfo.InitialData = m_Info.InitialData;
		imageInfo.Width = m_Info.Width;
		imageInfo.Height = m_Info.Height;
		imageInfo.Layers = 6;
		imageInfo.GenerateMipLevels = m_Info.GenerateMipLevels;

		m_Image = Image::Create(imageInfo);

		SetSampler(m_Info.SamplerInfo);
	}

	VulkanTextureCube::~VulkanTextureCube()
	{
		Renderer::SubmitResourceFree([samplerInfo = m_Info.SamplerInfo, vkSampler = m_Sampler]()
		{
			VulkanContext::GetAllocator()->DestroySampler(samplerInfo, vkSampler);
		});
	}

	void VulkanTextureCube::Resize(uint32 width, uint32 height)
	{
		if (m_Info.Width == width && m_Info.Height == height)
			return;

		m_Info.Width = width;
		m_Info.Height = height;

		m_Image->Resize(width, height);
	}

	void VulkanTextureCube::SetSampler(const TextureSamplerCreateInfo& samplerInfo)
	{
		if (m_Sampler != VK_NULL_HANDLE)
		{
			Renderer::SubmitResourceFree([this]()
			{
				VulkanContext::GetAllocator()->DestroySampler(m_Info.SamplerInfo, m_Sampler);
			});
		}

		m_Info.SamplerInfo = samplerInfo;

		m_Sampler = VulkanContext::GetAllocator()->AllocateSampler(m_Info.SamplerInfo);
		m_DescriptorInfo.sampler = m_Sampler;
	}

	VkImage VulkanTextureCube::GetVulkanImage() const
	{
		return m_Image.As<VulkanImage>()->GetVulkanImage();
	}

	VkImageView VulkanTextureCube::GetVulkanImageView() const
	{
		return m_Image.As<VulkanImage>()->GetVulkanImageView();
	}

	const VkDescriptorImageInfo& VulkanTextureCube::GetVulkanDescriptorInfo(uint32 mip)
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
