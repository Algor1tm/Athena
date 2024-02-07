#include "VulkanTextureCube.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"


namespace Athena
{
	VulkanTextureCube::VulkanTextureCube(const TextureCubeCreateInfo& info)
	{
		m_Info = info;

		if (info.MipLevels == 0)
			m_Info.MipLevels = Math::Floor(Math::Log2(Math::Max((float)info.Width, (float)info.Height))) + 1;

		m_DescriptorInfo.imageLayout = m_Info.Usage & ImageUsage::STORAGE ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_DescriptorInfo.imageView = VK_NULL_HANDLE;
		m_DescriptorInfo.sampler = VK_NULL_HANDLE;

		ImageCreateInfo imageInfo;
		imageInfo.Name = m_Info.Name;
		imageInfo.Format = m_Info.Format;
		imageInfo.Usage = m_Info.Usage;
		imageInfo.Type = ImageType::IMAGE_CUBE;
		imageInfo.InitialData = m_Info.InitialData;
		imageInfo.Width = m_Info.Width;
		imageInfo.Height = m_Info.Height;
		imageInfo.Layers = 6;
		imageInfo.MipLevels = m_Info.MipLevels;

		m_Image = Image::Create(imageInfo);

		SetSampler(m_Info.SamplerInfo);
	}

	VulkanTextureCube::~VulkanTextureCube()
	{
		Renderer::SubmitResourceFree([vkSampler = m_Sampler]()
		{
			vkDestroySampler(VulkanContext::GetLogicalDevice(), vkSampler, nullptr);
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

	ShaderResourceType VulkanTextureCube::GetResourceType()
	{
		// TODO
		if (m_Info.Usage & ImageUsage::STORAGE)
			return ShaderResourceType::StorageTextureCube;

		return ShaderResourceType::TextureCube;
	}

	VkImage VulkanTextureCube::GetVulkanImage() const
	{
		return m_Image.As<VulkanImage>()->GetVulkanImage();
	}

	VkImageView VulkanTextureCube::GetVulkanImageView() const
	{
		return m_Image.As<VulkanImage>()->GetVulkanImageView();
	}

	const VkDescriptorImageInfo& VulkanTextureCube::GetVulkanDescriptorInfo()
	{
		m_DescriptorInfo.imageView = GetVulkanImageView();
		return m_DescriptorInfo;
	}
}	