#include "VulkanTexture.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"


namespace Athena
{
	namespace Vulkan
	{
		static VkImageUsageFlags TranslateImageUsage(TextureUsage usage, Format format)
		{
			auto& formatProperties = GetFormatProperties(format);
			bool depthStencil = formatProperties.HasDepth || formatProperties.HasStencil;

			VkImageUsageFlags flags = 0;

			if (usage & TextureUsage::SAMPLED)
				flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

			if (usage & TextureUsage::UAV)
				flags |= VK_IMAGE_USAGE_STORAGE_BIT;

			if (usage & TextureUsage::ATTACHMENT)
				flags |= depthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			return (VkImageUsageFlags)flags;
		}

		inline VkImageType TranslateTextureType(TextureType type)
		{
			switch (type)
			{
			case TextureType::TEXTURE_2D:   return VK_IMAGE_TYPE_2D;
			case TextureType::TEXTURE_CUBE: return VK_IMAGE_TYPE_2D;
			}

			checkf(false);
			return (VkImageType)0;
		}

		inline VkSampleCountFlagBits TranslateSamplesCount(uint32 samples)
		{
			switch (samples)
			{
			case 1: return VK_SAMPLE_COUNT_1_BIT;
			case 2: return VK_SAMPLE_COUNT_2_BIT;
			case 4: return VK_SAMPLE_COUNT_4_BIT;
			case 8: return VK_SAMPLE_COUNT_8_BIT;
			case 16: return VK_SAMPLE_COUNT_16_BIT;
			}

			checkf(false);
			return VK_SAMPLE_COUNT_1_BIT;
		}

		static const char* ViewTypeToString(TextureViewType type)
		{
			switch (type)
			{
			case TextureViewType::SRV: return "SRV";
			case TextureViewType::UAV: return "UAV";
			case TextureViewType::RTV: return "RTV";
			case TextureViewType::DSV: return "DSV";
			case TextureViewType::DSV_READONLY: return "DSV";
			}

			checkf(false);
			return "Unknown";
		}
	}

	VulkanTexture::VulkanTexture(const TextureCreateInfo& info)
		: Texture(info)
	{

	}

	VulkanTexture::~VulkanTexture()
	{
		CleanUp();
	}

	void VulkanTexture::Init()
	{
		TRACY_PROFILE_FUNC();

		Resize(GetWidth(), GetHeight());
	}

	void VulkanTexture::Resize(uint32 width, uint32 height)
	{
		TRACY_PROFILE_FUNC();

		Texture::Resize(width, height);

		CleanUp();

		VkImageUsageFlags imageUsage = Vulkan::TranslateImageUsage(GetInfo().Usage, GetInfo().TextureFormat);

		imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.flags = GetType() == TextureType::TEXTURE_CUBE ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
		imageInfo.imageType = Vulkan::TranslateTextureType(GetType());
		imageInfo.extent.width = GetWidth();
		imageInfo.extent.height = GetHeight();
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = GetMipLevelsCount();
		imageInfo.arrayLayers = GetInfo().Layers * Vulkan::GetLayerMultiplier(GetType());
		imageInfo.format = Vulkan::TranslateFormat(GetInfo().TextureFormat);
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = imageUsage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = Vulkan::TranslateSamplesCount(GetInfo().Samples);

		VK_CHECK(VulkanRHI::Get()->GetAllocator()->AllocateImage(&m_Image, imageInfo, VMA_MEMORY_USAGE_AUTO, 0, GetName()));
		VK_SET_OBJECT_NAME(m_Image.GetImage(), VK_OBJECT_TYPE_IMAGE, fmt::format("Image_{}", GetName()));

		m_SubresourcesLayouts.resize(imageInfo.mipLevels * imageInfo.arrayLayers, imageInfo.initialLayout);
		m_MainLayout = imageInfo.initialLayout;
		m_LayoutsCollapsed = true;

		InvalidateViews();
	}

	VkImageLayout VulkanTexture::GetSubresLayout(uint32 mip, uint32 layer) const
	{
		if (m_LayoutsCollapsed)
			return m_MainLayout;

		return m_SubresourcesLayouts[GetLayers() * mip + layer];
	}

	void VulkanTexture::SetSubresLayout(VkImageLayout layout, const VkImageSubresourceRange& subresourceRange)
	{
		for (uint32 mip = subresourceRange.baseMipLevel; mip < subresourceRange.levelCount; ++mip)
		{
			for (uint32 layer = subresourceRange.baseArrayLayer; layer < subresourceRange.layerCount; ++layer)
			{
				m_SubresourcesLayouts[GetLayers() * mip + layer] = layout;
			}
		}

		CollapseSubresLayoutsIfSame();
	}

	bool VulkanTexture::AreSubresourcesSameLayout(const VkImageSubresourceRange& subresourceRange) const
	{
		if (m_LayoutsCollapsed)
			return true;

		VkImageLayout layout = m_SubresourcesLayouts[GetLayers() * subresourceRange.baseMipLevel + subresourceRange.baseArrayLayer];

		for (uint32 mip = subresourceRange.baseMipLevel; mip < subresourceRange.levelCount; ++mip)
		{
			for (uint32 layer = subresourceRange.baseArrayLayer; layer < subresourceRange.layerCount; ++layer)
			{
				if (layout != m_SubresourcesLayouts[GetLayers() * mip + layer])
					return false;
			}
		}

		return true;
	}

	void VulkanTexture::CollapseSubresLayoutsIfSame()
	{
		VkImageLayout layout = m_SubresourcesLayouts[0];

		for (uint32 i = 0; i < m_SubresourcesLayouts.size(); ++i)
		{
			if (layout != m_SubresourcesLayouts[i])
			{
				m_LayoutsCollapsed = false;
				return;
			}
		}

		m_MainLayout = layout;
		m_LayoutsCollapsed = true;
	}

	void VulkanTexture::CleanUp()
	{
		if (m_Image)
		{
			VulkanRHI::Get()->DeleteFrameResourcesDeferred([image = m_Image, name = GetName()]()
			{
				VulkanRHI::Get()->GetAllocator()->DestroyImage(image, name);
			});
		}
	}


	VulkanTextureView::VulkanTextureView(const TextureViewCreateInfo & info, Texture * texture)
		: TextureView(info, texture)
	{
		TRACY_PROFILE_FUNC();

		if (GetInfo().Name.empty())
		{
			const auto& info = GetInfo();

			m_Name = fmt::format("{}_{}_Mip[{},{}]_Layer[{},{}]", GetTexture()->GetName(), Vulkan::ViewTypeToString(GetViewType()),
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
		uint32 viewBaseMipLevel = GetRange().BaseMipLevel;
		uint32 viewMipLevelCount = GetRange().MipLevelCount;

		if (viewBaseMipLevel >= texMipLevelsCount)
			viewBaseMipLevel = texMipLevelsCount - 1;

		if (viewBaseMipLevel + viewMipLevelCount > texMipLevelsCount)
			viewMipLevelCount = texMipLevelsCount - viewBaseMipLevel;

		uint32 layersCount = GetRange().LayerCount * Vulkan::GetLayerMultiplier(GetTextureType());;

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = GetVulkanTexture()->GetImage();
		viewInfo.viewType = Vulkan::TranslateImageViewType(GetTextureType(), layersCount);
		viewInfo.format = Vulkan::TranslateFormat(format);
		viewInfo.subresourceRange.aspectMask = Vulkan::TranslateTextureAspect(GetRange().Aspect, format);
		viewInfo.subresourceRange.baseMipLevel = viewBaseMipLevel;
		viewInfo.subresourceRange.levelCount = viewMipLevelCount;
		viewInfo.subresourceRange.baseArrayLayer = GetRange().BaseLayer * Vulkan::GetLayerMultiplier(GetTextureType());
		viewInfo.subresourceRange.layerCount = layersCount;
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