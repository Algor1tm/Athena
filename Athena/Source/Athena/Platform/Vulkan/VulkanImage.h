#include "Athena/Core/Core.h"
#include "Athena/Renderer/Image.h"

#include <vulkan/vulkan.h>



namespace Athena
{
	class VulkanImage : public Image
	{
	public:
		VulkanImage(const ImageCreateInfo& info);
		~VulkanImage();

		virtual uint32 GetWidth() const override { return m_Width; }
		virtual uint32 GetHeight() const override { return m_Height; }

		virtual void* GetDescriptorSet() override { return m_DescriptorSet; }

		VkImageView GetVulkanImageView() const { return m_VkImageView; }

	private:
		VkDescriptorSet m_DescriptorSet;
		VkSampler m_Sampler;
		VkImageView m_VkImageView;
		VkImage m_VkImage;
		VkDeviceMemory m_ImageMemory;

		uint32 m_Width;
		uint32 m_Height;
	};
}
