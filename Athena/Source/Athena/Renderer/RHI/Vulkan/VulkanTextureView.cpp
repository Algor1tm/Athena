#include "VulkanTextureView.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanTextureView::VulkanTextureView(const TextureViewCreateInfo& info, Texture* texture)
		: TextureView(info, texture)
	{
		TRACY_PROFILE_FUNC();

		if (GetInfo().Name.empty())
		{
			const auto& info = GetInfo();

			m_Name = fmt::format("{}_Mip[{},{}]_Layer[{},{}]", GetTexture()->GetName(),
				info.Range.BaseMipLevel, info.Range.BaseMipLevel + info.Range.MipLevelCount - 1,
				info.Range.BaseLayer, info.Range.BaseLayer + info.Range.LayerCount - 1);
		}
		else
		{
			m_Name = GetInfo().Name;
		}

		if (!GetInfo().OverrideSampler)
		{
			SetSampler(GetTexture()->GetInfo().Sampler);
		}
	}

	VulkanTextureView::~VulkanTextureView()
	{
		CleanUp();

		if (m_Sampler)
		{
			VulkanRHI::Get()->DeleteFrameResourcesDeferred([this, samplerInfo = GetInfo().Sampler, vkSampler = m_Sampler]()
				{
					VulkanRHI::Get()->GetAllocator()->DestroySampler(samplerInfo, vkSampler);
				});
		}
	}

	void VulkanTextureView::Init()
	{
		TRACY_PROFILE_FUNC();

		m_Sampler = VulkanRHI::Get()->GetAllocator()->CreateSampler(GetInfo().Sampler);
		m_DescriptorInfo.sampler = m_Sampler;

		Invalidate();
	}

	void VulkanTextureView::Invalidate()
	{
		TRACY_PROFILE_FUNC();

		if (m_ImageView)
		{
			CleanUp();
		}

		if (GetTexture() == nullptr)
		{
			checkf(false);
			return;
		}

		Format format = GetTexture()->GetFormat();

		VkComponentMapping swizzling = {};
		swizzling.r = Vulkan::TranslateSwizzle(GetInfo().Swizzle.R);
		swizzling.g = Vulkan::TranslateSwizzle(GetInfo().Swizzle.G);
		swizzling.b = Vulkan::TranslateSwizzle(GetInfo().Swizzle.B);
		swizzling.a = Vulkan::TranslateSwizzle(GetInfo().Swizzle.A);

		uint32 texMipLevelsCount = GetTexture()->GetMipLevelsCount();
		uint32 viewBaseMipLevel = GetInfo().Range.BaseMipLevel;
		uint32 viewMipLevelCount = GetInfo().Range.MipLevelCount;

		if (viewBaseMipLevel >= texMipLevelsCount)
			viewBaseMipLevel = texMipLevelsCount - 1;

		if (viewBaseMipLevel + viewMipLevelCount > texMipLevelsCount)
			viewMipLevelCount = texMipLevelsCount - viewBaseMipLevel;

		uint32 layersCount = GetInfo().Range.LayerCount;

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = GetVulkanTexture()->GetImage();
		viewInfo.viewType = Vulkan::TranslateImageViewType(GetInfo().ViewType, layersCount);
		viewInfo.format = Vulkan::TranslateFormat(format);
		viewInfo.subresourceRange.aspectMask = Vulkan::TranslateTextureAspect(GetInfo().Range.Aspect, format);
		viewInfo.subresourceRange.baseMipLevel = viewBaseMipLevel;
		viewInfo.subresourceRange.levelCount = viewMipLevelCount;
		viewInfo.subresourceRange.baseArrayLayer = GetInfo().Range.BaseLayer;
		viewInfo.subresourceRange.layerCount = GetInfo().Range.LayerCount;
		viewInfo.components = swizzling;

		VK_CHECK(vkCreateImageView(Vulkan::GetLogicalDevice(), &viewInfo, nullptr, &m_ImageView));
		VK_SET_OBJECT_NAME(m_ImageView, VK_OBJECT_TYPE_IMAGE_VIEW, m_Name);
	}

	void VulkanTextureView::CleanUp()
	{
		if (m_ImageView)
		{
			VulkanRHI::Get()->DeleteFrameResourcesDeferred([this, imageView = m_ImageView]()
			{
				vkDestroyImageView(Vulkan::GetLogicalDevice(), imageView, nullptr);
			});
		}
	}

	VulkanTexture* VulkanTextureView::GetVulkanTexture() const
	{
		return reinterpret_cast<VulkanTexture*>(GetTexture());
	}

	const VkDescriptorImageInfo& VulkanTextureView::GetVulkanDescriptorInfo()
	{
		m_DescriptorInfo.imageView = m_ImageView;
		m_DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		// Set default layout if image has not initalized yet
		if (m_DescriptorInfo.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED)
			m_DescriptorInfo.imageLayout = GetTexture()->GetInfo().Usage & TextureUsage::UAV ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		return m_DescriptorInfo;
	}
}
