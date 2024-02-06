#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Image.h"
#include "Athena/Platform/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanImage : public Image
	{
	public:
		VulkanImage(const ImageCreateInfo& info);
		~VulkanImage();

		virtual void Resize(uint32 width, uint32 height) override;
		virtual void GenerateMipMap(uint32 levels) override;

		VkImage GetVulkanImage() const { return m_Image.GetImage(); }
		VkImageView GetVulkanImageView() const { return m_ImageView; }

	private:
		void RT_UploadData(const void* data, uint32 width, uint32 height);
		void CleanUp();

	private:
		VulkanImageAllocation m_Image;
		VkImageView m_ImageView = VK_NULL_HANDLE;
	};
}
