#pragma once
#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(const Texture2DCreateInfo& info);
		VulkanTexture2D(const Ref<Image>& image, const TextureSamplerCreateInfo& samplerInfo);
		~VulkanTexture2D();

		virtual void Resize(uint32 width, uint32 height) override;
		virtual void SetSampler(const TextureSamplerCreateInfo& samplerInfo) override;

		VkSampler GetVulkanSampler() const { return m_Sampler; }
		VkImage GetVulkanImage() const;
		VkImageView GetVulkanImageView() const;
		const VkDescriptorImageInfo& GetVulkanDescriptorInfo(uint32 mip = 0);

	private:
		VkSampler m_Sampler = VK_NULL_HANDLE;
		VkDescriptorImageInfo m_DescriptorInfo;
	};
}
