#include "Texture.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanTextureCube.h"
#include "Athena/Platform/Vulkan/VulkanTextureView.h"


namespace Athena
{
	Vector2u Texture::GetMipSize(uint32 mip) const
	{
		uint32 width = GetWidth();
		uint32 height = GetHeight();

		Vector2u mipSize;
		mipSize.x = float(width) * Math::Pow<float>(0.5f, mip);
		mipSize.y = float(height) * Math::Pow<float>(0.5f, mip);

		return mipSize;
	}

	uint32 Texture::GetMipLevelsCount() const
	{
		return Math::Floor(Math::Log2(Math::Max<float>(GetWidth(), GetHeight()))) + 1;
	}

	Ref<TextureView> TextureView::Create(const Ref<Texture>& texture, const TextureViewCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanTextureView>::Create(texture, info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Texture::~Texture()
	{
		ClearViews();
	}

	Ref<TextureView> Texture::GetMipView(uint32 mip)
	{
		TextureViewCreateInfo info;
		info.BaseMipLevel = mip;
		info.MipLevelCount = 1;

		if (GetType() == TextureType::TEXTURE_CUBE)
			info.LayerCount = 6;

		return GetView(info);
	}

	Ref<TextureView> Texture::GetLayerView(uint32 layer)
	{
		TextureViewCreateInfo info;

		if (GetType() == TextureType::TEXTURE_2D)
		{
			info.BaseLayer = layer;
			info.LayerCount = 1;
		}
		else
		{
			info.BaseLayer = layer * 6;
			info.LayerCount = 6;
		}

		return GetView(info);
	}

	Ref<TextureView> Texture::GetView(const TextureViewCreateInfo& info)
	{
		if (m_TextureViews.contains(info))
			return m_TextureViews.at(info);

		m_TextureViews[info] = TextureView::Create(Ref(this), info);
		return m_TextureViews.at(info);
	}

	void Texture::InvalidateViews()
	{
		// Remove mip views if they exceed max mip level
		std::vector<TextureViewCreateInfo> viewsToRemove;
		for (const auto& [info, _] : m_TextureViews)
		{
			if (info.BaseMipLevel + info.MipLevelCount > GetMipLevelsCount())
				viewsToRemove.push_back(info);
		}

		for (const auto& info : viewsToRemove)
		{
			m_TextureViews.erase(info);
		}

		for (auto& [info, view] : m_TextureViews)
		{
			view->Invalidate();
		}
	}

	void Texture::ClearViews()
	{
		m_TextureViews.clear();
	}


	Ref<Texture2D> Texture2D::Create(const TextureCreateInfo& info, Buffer data)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanTexture2D>::Create(info, data);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Ref<TextureCube> TextureCube::Create(const TextureCreateInfo& info, Buffer data)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanTextureCube>::Create(info, data);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Texture2DInstance::Texture2DInstance()
	{
		SetTexCoords({ Vector2{0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} });
	}

	Texture2DInstance::Texture2DInstance(const Ref<Texture2D>& texture)
	{
		SetTexture(texture);
		SetTexCoords({ Vector2{0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} });
	}

	Texture2DInstance::Texture2DInstance(const Ref<Texture2D>& texture, const std::array<Vector2, 4>& texCoords)
	{
		SetTexture(texture);
		SetTexCoords(texCoords);
	}

	Texture2DInstance::Texture2DInstance(const Ref<Texture2D>& texture, const Vector2& min, const Vector2& max)
	{
		SetTexture(texture);
		SetTexCoords(min, max);
	}

	void Texture2DInstance::SetTexCoords(const Vector2& min, const Vector2& max)
	{
		float width = (float)m_Texture->GetInfo().Width;
		float height = (float)m_Texture->GetInfo().Height;

		m_TexCoords[0] = { min.x / width, min.y / height };
		m_TexCoords[1] = { max.x / width, min.y / height };
		m_TexCoords[2] = { max.x / width, max.y / height };
		m_TexCoords[3] = { min.x / width, max.y / height };
	}
}
