#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"

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

		VkImageView GetVulkanImageView() const { return m_VkImageView; }

	private:
		void UploadData(const void* data, uint32 width, uint32 height);

	private:
		VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
		VkImage m_VkImage = VK_NULL_HANDLE;
		VkImageView m_VkImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;

		VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
	};
}
