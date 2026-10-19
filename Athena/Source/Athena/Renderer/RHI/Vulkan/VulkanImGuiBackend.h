#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Hash.h"
#include "Athena/UI/ImGuiLayer.h"
#include "Athena/Renderer/Texture.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	struct TextureInfo
	{
		VkImageView VulkanImageView = VK_NULL_HANDLE;
		VkSampler VulkanSampler = VK_NULL_HANDLE;
		VkDescriptorSet Set = VK_NULL_HANDLE;
	};

	class ATHENA_API VulkanImGuiBackend: public ImGuiBackend
	{
	public:
		VulkanImGuiBackend();
		virtual ~VulkanImGuiBackend();

		virtual void Init(void* windowHandle) override;

		virtual void NewFrame() override;
		virtual void RenderDrawData(uint32 width, uint32 height) override;
		virtual void RenderViewports() override;

		virtual void* GetTextureID(const Ref<TextureView>& texture) override;

		virtual void OnSwapChainRecreate() override;

	private:
		void RecreateFramebuffers();
		void InvalidateDescriptorSets();
		void RemoveDescriptorSet(VkDescriptorSet set);

	private:
		VkDescriptorPool m_ImGuiDescriptorPool;
		VkRenderPass m_ImGuiRenderPass;
		std::vector<VkFramebuffer> m_SwapChainFramebuffers;

		std::unordered_map<Ref<TextureView>, TextureInfo> m_TextureViewsMap;
	};
}
