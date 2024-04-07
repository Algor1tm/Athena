#pragma once
#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(const TextureCreateInfo& info, Buffer data);
		~VulkanTexture2D();

		virtual void Resize(uint32 width, uint32 height) override;
		virtual void SetSampler(const TextureSamplerCreateInfo& samplerInfo) override;

		virtual void WriteContentToBuffer(Buffer* dstBuffer) override;

		Ref<VulkanImage> GetImage() const { return m_Image; }

		VkSampler GetVulkanSampler() const { return m_Sampler; }
		VkImage GetVulkanImage() const;
		VkImageView GetVulkanImageView() const;
		const VkDescriptorImageInfo& GetVulkanDescriptorInfo();

	private:
		Ref<VulkanImage> m_Image;
		VkSampler m_Sampler;
		VkDescriptorImageInfo m_DescriptorInfo;
	};
}
