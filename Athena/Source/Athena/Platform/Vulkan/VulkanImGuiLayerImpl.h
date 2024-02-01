#pragma once

#include "Athena/Core/Core.h"
#include "Athena/ImGui/ImGuiLayer.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"

#include <vulkan/vulkan.h>


namespace std
{
	// TODO: need to properly hash textures, 
	// for now there is no garantuee, that there are no textures with same names
	template<>
	struct hash<Athena::Ref<Athena::Texture2D>>
	{
		size_t operator()(const Athena::Ref<Athena::Texture2D>& tex) const
		{
			const std::string& name = tex.As<Athena::VulkanTexture2D>()->GetInfo().Name;
			return hash<string>()(name);
		}
	};
}


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

		virtual void* GetTextureID(Ref<Texture2D> texture) override;

	private:
		void RecreateFramebuffers();
		void InvalidateDescriptorSets();
		void RemoveDescriptorSet(VkDescriptorSet set);

	private:
		struct TextureInfo
		{
			VkImageView ImageView;
			VkSampler Sampler;
			VkDescriptorSet Set;
		};

	private:
		VkDescriptorPool m_ImGuiDescriptorPool;
		VkRenderPass m_ImGuiRenderPass;
		std::vector<VkFramebuffer> m_SwapChainFramebuffers;
		std::unordered_map<Ref<Texture2D>, TextureInfo> m_TextureMap;
	};
}
