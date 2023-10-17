#pragma once

#include "Athena/Core/Core.h"
#include "Athena/ImGui/ImGuiLayer.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanImGuiLayerImpl : public ImGuiLayerImpl
	{
	public:
		virtual void Init(void* windowHandle) override;
		virtual void Shutdown() override;

		virtual void NewFrame() override;
		virtual void RenderDrawData(uint32 width, uint32 height) override;

		virtual void OnSwapChainRecreate() override;

	private:
		void RecreateFramebuffers();

	private:
		VkDescriptorPool m_ImGuiDescriptorPool;
		VkRenderPass m_ImGuiRenderPass;
		std::vector<VkFramebuffer> m_SwapChainFramebuffers;
	};
}
