#include "VulkanSampler.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanSampler::VulkanSampler(const TextureSamplerCreateInfo& info)
	{
		m_Info = info;
		m_VulkanSampler = VK_NULL_HANDLE;

		m_DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_DescriptorInfo.imageView = VK_NULL_HANDLE;

		Recreate();
	}

	VulkanSampler::~VulkanSampler()
	{
		CleanUp();
	}

	void VulkanSampler::CleanUp()
	{
		if (m_VulkanSampler != VK_NULL_HANDLE)
		{
			Renderer::SubmitResourceFree([vkSampler = m_VulkanSampler]()
			{
				vkDestroySampler(VulkanContext::GetLogicalDevice(), vkSampler, nullptr);
			});
		}
	}

	void VulkanSampler::Recreate()
	{
		CleanUp();

		Renderer::Submit([this]()
		{
			VkSamplerCreateInfo vksamplerInfo = {};
			vksamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			vksamplerInfo.magFilter = Vulkan::GetFilter(m_Info.MagFilter);
			vksamplerInfo.minFilter = Vulkan::GetFilter(m_Info.MinFilter);
			vksamplerInfo.mipmapMode = Vulkan::GetMipMapMode(m_Info.MipMapFilter);
			vksamplerInfo.addressModeU = Vulkan::GetWrap(m_Info.Wrap);
			vksamplerInfo.addressModeV = Vulkan::GetWrap(m_Info.Wrap);
			vksamplerInfo.addressModeW = Vulkan::GetWrap(m_Info.Wrap);
			vksamplerInfo.anisotropyEnable = false;
			vksamplerInfo.maxAnisotropy = 1.f;
			vksamplerInfo.compareEnable = m_Info.Compare == TextureCompareOperator::NONE ? false : true;
			vksamplerInfo.compareOp = Vulkan::GetCompareOp(m_Info.Compare);
			vksamplerInfo.minLod = 0.f;
			vksamplerInfo.maxLod = 1.f;
			vksamplerInfo.mipLodBias = 0.f;
			vksamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			vksamplerInfo.unnormalizedCoordinates = false;

			VK_CHECK(vkCreateSampler(VulkanContext::GetLogicalDevice(), &vksamplerInfo, nullptr, &m_VulkanSampler));
			m_DescriptorInfo.sampler = m_VulkanSampler;
		});
	}

	VkSampler VulkanSampler::GetVulkanSampler()
	{
		return m_VulkanSampler;
	}

	const VkDescriptorImageInfo& VulkanSampler::GetVulkanDescriptorInfo()
	{
		return m_DescriptorInfo;
	}
}
