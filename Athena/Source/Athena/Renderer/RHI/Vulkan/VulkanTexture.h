#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanTexture : public Texture
	{
	public:
		VulkanTexture(const TextureCreateInfo& info);
		virtual ~VulkanTexture();

		virtual void Init() override;
		virtual void Resize(uint32 width, uint32 height) override;
		virtual void Upload(const Buffer& srcBuffer, const TextureRegion& region) override;
		virtual void UploadAndBlitMipMap(const Buffer& srcBuffer) override;

		VkImage GetImage() const { return m_Image.GetImage(); }

	private:
		void CleanUp();

	private:
		VulkanImageAllocation m_Image;
	};


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
