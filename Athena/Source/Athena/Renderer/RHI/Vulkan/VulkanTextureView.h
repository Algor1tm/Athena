#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanAllocator.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanTexture.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanTextureView : public TextureView
	{
	public:
		VulkanTextureView(const TextureViewCreateInfo& info, Texture* texture);
		virtual ~VulkanTextureView();
		
		virtual void Init() override;
		virtual void Invalidate() override;

		VulkanTexture* GetVulkanTexture() const;
		VkImageView GetImageView() const { return m_ImageView; }
		VkSampler GetSampler() const { return m_Sampler; }

		const VkDescriptorImageInfo& GetVulkanDescriptorInfo();

	private:
		void CleanUp();

	private:
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		VkDescriptorImageInfo m_DescriptorInfo;
		String m_Name;
	};
}
