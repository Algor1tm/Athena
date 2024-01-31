#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Framebuffer.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanFramebuffer: public Framebuffer
	{
	public:
		VulkanFramebuffer(const FramebufferCreateInfo& info);
		~VulkanFramebuffer();

		virtual void Resize(uint32 width, uint32 height) override;

		virtual const Ref<Texture2D>& GetColorAttachment(uint32 index = 0) const override;
		virtual const Ref<Texture2D>& GetDepthAttachment() const override;

		virtual uint32 GetColorAttachmentCount() override;
		virtual bool HasDepthAttachment() override;

		VkFramebuffer GetVulkanFramebuffer() const;

		void RT_PrepareFramebuffer(VkRenderPass renderPass);

	private:
		void CleanUpFramebufferSet();
		void ResizeFramebufferSet();

	private:
		std::vector<std::vector<Ref<Texture2D>>> m_ColorAttachmentsSet;
		std::vector<Ref<Texture2D>> m_DepthAttachmentSet;
		std::vector<VkFramebuffer> m_VulkanFramebufferSet;
		VkRenderPass m_VulkanRenderPass;
	};
}
