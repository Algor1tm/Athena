#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Platform/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class ATHENA_API VulkanTextureCube : public TextureCube
	{
	public:
		VulkanTextureCube(const TextureCubeCreateInfo& info);
		~VulkanTextureCube();

		virtual void Resize(uint32 width, uint32 height) override;
		virtual void SetSampler(const TextureSamplerCreateInfo& samplerInfo) override;

		VkSampler GetVulkanSampler() const { return m_Sampler; }
		VkImage GetVulkanImage() const;
		VkImageView GetVulkanImageView() const;
		const VkDescriptorImageInfo& GetVulkanDescriptorInfo();

	private:
		VkSampler m_Sampler = VK_NULL_HANDLE;
		VkDescriptorImageInfo m_DescriptorInfo;
	};
}
