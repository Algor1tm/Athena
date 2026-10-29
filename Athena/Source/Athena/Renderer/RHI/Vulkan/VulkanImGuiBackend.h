#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Hash.h"
#include "Athena/UI/ImGuiBackend.h"
#include "Athena/Renderer/Texture.h"

#include "imgui_threaded_rendering.h"

#include <mutex>
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

		virtual void Init_GT(void* windowHandle) override;
		virtual void Init_RT() override;

		virtual void NewFrame_GT() override;
		virtual void NewFrame_RT() override;
		virtual void SwapDrawData() override;
		virtual void RenderDrawData() override;

		virtual void* GetTextureID(const Ref<TextureView>& texture) override;

		virtual void OnSwapChainRecreate() override;

	private:
		void RecreateFramebuffers();
		void InvalidateDescriptorSets();
		void RemoveDescriptorSet(VkDescriptorSet set);

		void UpdateTextureID_RT(const Ref<TextureView>& view);

	private:
		VkDescriptorPool m_ImGuiDescriptorPool;
		VkRenderPass m_ImGuiRenderPass;
		std::vector<VkFramebuffer> m_SwapChainFramebuffers;

		ImTextureQueue m_ImGuiTextureQueue;
		std::mutex m_TextureQueueMutex;

		std::unordered_map<Ref<TextureView>, TextureInfo> m_TextureViewsMap;
		std::mutex m_TexturesMapMutex;
	};
}
