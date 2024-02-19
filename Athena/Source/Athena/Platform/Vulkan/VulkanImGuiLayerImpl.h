#pragma once

#include "Athena/Core/Core.h"
#include "Athena/ImGui/ImGuiLayer.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	struct ImageView
	{
		Ref<Image> Image;
		uint32 MipLevel = 0;
		uint32 Layer = 0;

		bool operator==(const ImageView& other) const = default;
	};

	struct ImageInfo
	{
		VkImageView VulkanImageView = VK_NULL_HANDLE;
		VkDescriptorSet Set = VK_NULL_HANDLE;
	};
}


namespace std
{
	using namespace Athena;

	template<>
	struct hash<Ref<Image>>
	{
		size_t operator()(const Ref<Image>& value) const
		{
			return (size_t)value.Raw();
		}
	};

	template<>
	struct hash<ImageView>
	{
		size_t operator()(const ImageView& value) const
		{
			return (size_t)((byte*)value.Image.Raw() + value.Layer + value.MipLevel);
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

		virtual void* GetTextureID(const Ref<Image>& image) override;
		virtual void* GetTextureMipID(const Ref<Image>& image, uint32 mip) override;
		virtual void* GetTextureLayerID(const Ref<Image>& image, uint32 layer) override;

	private:
		void RecreateFramebuffers();
		void InvalidateDescriptorSets();
		void RemoveDescriptorSet(VkDescriptorSet set);

	private:
		VkDescriptorPool m_ImGuiDescriptorPool;
		VkRenderPass m_ImGuiRenderPass;

		std::vector<VkFramebuffer> m_SwapChainFramebuffers;
		std::unordered_map<Ref<Image>, ImageInfo> m_ImagesMap;
		std::unordered_map<ImageView, ImageInfo> m_ImageViewsMap;
		VkSampler m_UISampler;
	};
}
