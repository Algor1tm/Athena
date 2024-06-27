#include "VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"


namespace Athena
{
	VulkanTexture2D::VulkanTexture2D(const TextureCreateInfo& info, Buffer data)
	{
		m_Info = info;
		m_Sampler = VK_NULL_HANDLE;
		m_DescriptorInfo.sampler = m_Sampler;

		m_Image = Ref<VulkanImage>::Create(info, TextureType::TEXTURE_2D, data);

		SetSampler(m_Info.Sampler);
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		Renderer::SubmitResourceFree([samplerInfo = m_Info.Sampler, vkSampler = m_Sampler]()
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

		InvalidateViews();
	}

	void VulkanTexture2D::SetSampler(const TextureSamplerCreateInfo& samplerInfo)
	{
		if (m_Sampler != VK_NULL_HANDLE)
		{
			Renderer::SubmitResourceFree([this, samplerInfo = m_Info.Sampler, vkSampler = m_Sampler]()
			{
				VulkanContext::GetAllocator()->DestroySampler(samplerInfo, vkSampler);
			});
		}

		m_Info.Sampler = samplerInfo;

		m_Sampler = VulkanContext::GetAllocator()->CreateSampler(m_Info.Sampler);
		m_DescriptorInfo.sampler = m_Sampler;
	}

	void VulkanTexture2D::WriteContentToBuffer(Buffer* dstBuffer)
	{
		m_Image->WriteContentToBuffer(dstBuffer);
	}

	VkImage VulkanTexture2D::GetVulkanImage() const 
	{
		return m_Image->GetVulkanImage();
	}

	VkImageView VulkanTexture2D::GetVulkanImageView() const
	{ 
		return m_Image->GetVulkanImageView();
	}

	const VkDescriptorImageInfo& VulkanTexture2D::GetVulkanDescriptorInfo()
	{
		m_DescriptorInfo.imageView = GetVulkanImageView();
		m_DescriptorInfo.imageLayout = m_Image->GetLayout();

		// Set default layout if image has not initalized yet
		if (m_DescriptorInfo.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED)
			m_DescriptorInfo.imageLayout = m_Info.Usage & TextureUsage::STORAGE ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		return m_DescriptorInfo;
	}
}
