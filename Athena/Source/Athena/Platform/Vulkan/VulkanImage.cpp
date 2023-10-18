#include "VulkanImage.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"

#include <ImGui/backends/imgui_impl_vulkan.h>


namespace Athena
{
	VulkanImage::VulkanImage(const ImageCreateInfo& info)
	{
		m_Width = info.Width;
		m_Height = info.Height;

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = info.Width;
		imageInfo.extent.height = info.Height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = Utils::GetVulkanFormat(info.Format);
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		VK_CHECK(vkCreateImage(VulkanContext::GetLogicalDevice(), &imageInfo, VulkanContext::GetAllocator(), &m_VkImage));


		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(VulkanContext::GetLogicalDevice(), m_VkImage, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = Utils::GetVulkanMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VK_CHECK(vkAllocateMemory(VulkanContext::GetLogicalDevice(), &allocInfo, VulkanContext::GetAllocator(), &m_ImageMemory));

		vkBindImageMemory(VulkanContext::GetLogicalDevice(), m_VkImage, m_ImageMemory, 0);


		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_VkImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = Utils::GetVulkanFormat(info.Format);
		viewInfo.subresourceRange.aspectMask = Utils::GetVulkanImageAspect(info.Format);
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VK_CHECK(vkCreateImageView(VulkanContext::GetLogicalDevice(), &viewInfo, VulkanContext::GetAllocator(), &m_VkImageView));


		// Create Sampler
		{
			VkSamplerCreateInfo samplerInfo = {};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.mipLodBias = 0.f;
			samplerInfo.anisotropyEnable = false;
			samplerInfo.maxAnisotropy = 1.0f;
			samplerInfo.compareEnable = false;
			samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
			samplerInfo.minLod = -1000;
			samplerInfo.maxLod = 1000;
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			samplerInfo.unnormalizedCoordinates = false;

			VK_CHECK(vkCreateSampler(VulkanContext::GetLogicalDevice(), &samplerInfo, VulkanContext::GetAllocator(), &m_Sampler));
		}

		m_DescriptorSet = ImGui_ImplVulkan_AddTexture(m_Sampler, m_VkImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	VulkanImage::~VulkanImage()
	{
		Renderer::SubmitResourceFree([vkSampler = m_Sampler, vkImageView = m_VkImageView, vkImage = m_VkImage, imageMemory = m_ImageMemory]()
			{
				vkDestroySampler(VulkanContext::GetLogicalDevice(), vkSampler, VulkanContext::GetAllocator());
				vkDestroyImageView(VulkanContext::GetLogicalDevice(), vkImageView, VulkanContext::GetAllocator());

				vkDestroyImage(VulkanContext::GetLogicalDevice(), vkImage, VulkanContext::GetAllocator());
				vkFreeMemory(VulkanContext::GetLogicalDevice(), imageMemory, VulkanContext::GetAllocator());
			});
	}
}
