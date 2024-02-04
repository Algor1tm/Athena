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

		virtual uint32 GetColorAttachmentCount() const override;
		virtual bool HasDepthAttachment() const override;

		VkFramebuffer GetVulkanFramebuffer() const;

		void Bake(VkRenderPass renderPass);

	private:
		void CleanUpFramebuffer();
		void ResizeFramebuffer();

	private:
		std::vector<Ref<Texture2D>> m_ColorAttachments;
		Ref<Texture2D> m_DepthAttachment;
		VkFramebuffer m_VulkanFramebuffer;
		VkRenderPass m_VulkanRenderPass;
	};
}
