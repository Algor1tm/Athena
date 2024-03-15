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

		virtual void BlitMipMap(uint32 levels) override;
		virtual void BlitMipMap(const Ref<RenderCommandBuffer>& cmdBuffer, uint32 levels) override;
		void BlitMipMap(VkCommandBuffer vkcmdBuffer, uint32 levels);

		virtual uint32 GetMipLevelsCount() const override { return m_MipLevels; }

		VkImage GetVulkanImage() const { return m_Image.GetImage(); }
		VkImageView GetVulkanImageView() const { return m_ImageView; }
		VkImageLayout GetLayout() const { return m_Layout; }

		VkImageView GetVulkanImageViewMip(uint32 mip) const { return m_ImageViewsPerMip[mip]; }
		VkImageView GetVulkanImageViewLayer(uint32 layer) const { return m_ImageViewsPerLayer[layer]; }

		void TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);

	private:
		void UploadData(const void* data, uint32 width, uint32 height);
		void CleanUp();

	private:
		uint32 m_MipLevels;
		VkImageLayout m_Layout;
		VulkanImageAllocation m_Image;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		std::vector<VkImageView> m_ImageViewsPerMip;
		std::vector<VkImageView> m_ImageViewsPerLayer;
	};
}
