#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RenderPass.h"

#include <vulkan/vulkan.h>

namespace Athena
{
	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(const RenderPassCreateInfo& info);
		~VulkanRenderPass();

		virtual void Begin(const Ref<RenderCommandBuffer>& commandBuffer) override;
		virtual void End(const Ref<RenderCommandBuffer>& commandBuffer) override;

		virtual void Resize(uint32 width, uint32 height) override;

		virtual Ref<Texture2D> GetOutput(uint32 attachmentIndex) const override;
		virtual Ref<Texture2D> GetDepthOutput() const override;

		virtual uint32 GetColorAttachmentCount() const override;
		virtual bool HasDepthAttachment() const override;

		VkRenderPass GetVulkanRenderPass() const { return m_VulkanRenderPass; };

	private:
		std::vector<Ref<Texture2D>> m_ColorAttachments;
		Ref<Texture2D> m_DepthAttachment;
		VkFramebuffer m_VulkanFramebuffer = VK_NULL_HANDLE;
		VkRenderPass m_VulkanRenderPass = VK_NULL_HANDLE;
		std::vector<VkClearValue> m_ClearColors;
	};
}
