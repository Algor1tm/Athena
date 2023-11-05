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

		virtual void* GetDescriptorSet() override { return m_DescriptorSet; }

		virtual void GenerateMipMap(uint32 levels) override;
		virtual void ResetSampler(const TextureSamplerCreateInfo& samplerInfo) override;

		VkImage GetVulkanImage() { return m_Image.GetImage(); }
		VkImageView GetVulkanImageView() { return m_ImageView; }

	private:
		void UploadData(const void* data, uint32 width, uint32 height);

	private:
		VulkanImage m_Image;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;

		VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
		bool m_AddImGuiTexture = false;
	};
}
