#include "VulkanTexture2D.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"

#include <ImGui/backends/imgui_impl_vulkan.h>


namespace Athena
{
	VulkanTexture2D::VulkanTexture2D(const TextureCreateInfo& info)
	{
		m_Info = info;

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = info.Width;
		imageInfo.extent.height = info.Height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = Utils::GetVulkanFormat(info.Format, info.sRGB);
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		VK_CHECK(vkCreateImage(VulkanContext::GetLogicalDevice(), &imageInfo, VulkanContext::GetAllocator(), &m_VkImage));


		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(VulkanContext::GetLogicalDevice(), m_VkImage, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = Utils::GetVulkanMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VK_CHECK(vkAllocateMemory(VulkanContext::GetLogicalDevice(), &allocInfo, VulkanContext::GetAllocator(), &m_ImageMemory));

		vkBindImageMemory(VulkanContext::GetLogicalDevice(), m_VkImage, m_ImageMemory, 0);


		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_VkImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = Utils::GetVulkanFormat(info.Format, info.sRGB);
		viewInfo.subresourceRange.aspectMask = Utils::GetVulkanImageAspectFlags(info.Format);
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VK_CHECK(vkCreateImageView(VulkanContext::GetLogicalDevice(), &viewInfo, VulkanContext::GetAllocator(), &m_VkImageView));

		ResetSampler(m_Info.SamplerInfo);
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		Renderer::SubmitResourceFree([set = m_DescriptorSet, vkSampler = m_Sampler, vkImageView = m_VkImageView, 
				vkImage = m_VkImage, imageMemory = m_ImageMemory]()
			{
				ImGui_ImplVulkan_RemoveTexture(set);

				vkDestroySampler(VulkanContext::GetLogicalDevice(), vkSampler, VulkanContext::GetAllocator());
				vkDestroyImageView(VulkanContext::GetLogicalDevice(), vkImageView, VulkanContext::GetAllocator());

				vkDestroyImage(VulkanContext::GetLogicalDevice(), vkImage, VulkanContext::GetAllocator());
				vkFreeMemory(VulkanContext::GetLogicalDevice(), imageMemory, VulkanContext::GetAllocator());
			});
	}

	void VulkanTexture2D::ResetSampler(const TextureSamplerCreateInfo& samplerInfo)
	{
		if (m_Sampler != VK_NULL_HANDLE)
		{
			Renderer::SubmitResourceFree([vkSampler = m_Sampler, set = m_DescriptorSet]()
				{
					vkDestroySampler(VulkanContext::GetLogicalDevice(), vkSampler, VulkanContext::GetAllocator());
					ImGui_ImplVulkan_RemoveTexture(set);
				});
		}

		// Create Sampler
		{
			VkSamplerCreateInfo vksamplerInfo = {};
			vksamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			vksamplerInfo.magFilter = Utils::GetVulkanFilter(samplerInfo.MagFilter);
			vksamplerInfo.minFilter = Utils::GetVulkanFilter(samplerInfo.MinFilter);
			vksamplerInfo.mipmapMode = Utils::GetVulkanMipMapMode(samplerInfo.MipMapFilter);
			vksamplerInfo.addressModeU = Utils::GetVulkanWrap(samplerInfo.Wrap);
			vksamplerInfo.addressModeV = Utils::GetVulkanWrap(samplerInfo.Wrap);
			vksamplerInfo.addressModeW = Utils::GetVulkanWrap(samplerInfo.Wrap);
			vksamplerInfo.mipLodBias = 0.f;
			vksamplerInfo.anisotropyEnable = false;
			vksamplerInfo.maxAnisotropy = 1.0f;
			vksamplerInfo.compareEnable = false;
			vksamplerInfo.compareOp = VK_COMPARE_OP_NEVER;
			vksamplerInfo.minLod = -1000;
			vksamplerInfo.maxLod = 1000;
			vksamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			vksamplerInfo.unnormalizedCoordinates = false;

			VK_CHECK(vkCreateSampler(VulkanContext::GetLogicalDevice(), &vksamplerInfo, VulkanContext::GetAllocator(), &m_Sampler));
		}

		m_DescriptorSet = ImGui_ImplVulkan_AddTexture(m_Sampler, m_VkImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void VulkanTexture2D::GenerateMipMap(uint32 maxLevel)
	{
		// TODO
	}
}
