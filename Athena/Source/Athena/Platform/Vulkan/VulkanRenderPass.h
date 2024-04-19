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
		virtual void Bake() override;

		VkRenderPass GetVulkanRenderPass() const { return m_VulkanRenderPass; };

	private:
		void BuildDependencies(std::vector<VkSubpassDependency>& dependencies);

	private:
		VkFramebuffer m_VulkanFramebuffer = VK_NULL_HANDLE;
		VkRenderPass m_VulkanRenderPass = VK_NULL_HANDLE;
		std::vector<VkClearValue> m_ClearColors;
		std::vector<VkImageLayout> m_InitalLayouts;
	};
}
