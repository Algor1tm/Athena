#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Platform/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>



namespace Athena
{
	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(const TextureCreateInfo& info);
		~VulkanTexture2D();

		virtual void Resize(uint32 width, uint32 height) override;

		virtual void SetSampler(const TextureSamplerCreateInfo& samplerInfo) override;
		virtual void GenerateMipMap(uint32 levels) override;

		virtual void* GetDescriptorSet() override;

		VkImage GetVulkanImage() { return m_Image.GetImage(); }
		VkImageView GetVulkanImageView() { return m_ImageView; }
		VkSampler GetVulkanSampler() { return m_Sampler; }

	private:
		void UploadData(const void* data, uint32 width, uint32 height);

	private:
		VulkanImage m_Image;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;

		VkDescriptorSet m_UIDescriptorSet = VK_NULL_HANDLE;
	};
}
