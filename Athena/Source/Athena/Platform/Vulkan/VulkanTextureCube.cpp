#include "VulkanTextureCube.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"


namespace Athena
{
	VulkanTextureCube::VulkanTextureCube(const TextureCreateInfo& info, Buffer data)
	{
		m_Info = info;
		m_Sampler = VK_NULL_HANDLE;
		m_DescriptorInfo.sampler = m_Sampler;

		m_Image = Ref<VulkanImage>::Create(info, TextureType::TEXTURE_CUBE, data);

		SetSampler(m_Info.Sampler);
	}

	VulkanTextureCube::~VulkanTextureCube()
	{
		Renderer::SubmitResourceFree([samplerInfo = m_Info.Sampler, vkSampler = m_Sampler]()
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

		InvalidateViews();
	}

	void VulkanTextureCube::SetSampler(const TextureSamplerCreateInfo& samplerInfo)
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

	void VulkanTextureCube::WriteContentToBuffer(Buffer* dstBuffer)
	{
		m_Image->WriteContentToBuffer(dstBuffer);
	}

	VkImage VulkanTextureCube::GetVulkanImage() const
	{
		return m_Image->GetVulkanImage();
	}

	VkImageView VulkanTextureCube::GetVulkanImageView() const
	{
		return m_Image->GetVulkanImageView();
	}

	const VkDescriptorImageInfo& VulkanTextureCube::GetVulkanDescriptorInfo()
	{
		m_DescriptorInfo.imageView = GetVulkanImageView();
		m_DescriptorInfo.imageLayout = m_Image->GetLayout();

		// Set default layout if image has not initalized yet
		if (m_DescriptorInfo.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED)
			m_DescriptorInfo.imageLayout = m_Info.Usage & TextureUsage::STORAGE ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		return m_DescriptorInfo;
	}
}	
