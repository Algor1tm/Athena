#include "Texture.h"

#include "Athena/Asset/Editor/TextureImporter.h"
#include "Athena/Renderer/RenderThread.h"
#include "Athena/Renderer/EngineRenderAssets.h"
#include "Athena/Renderer/RHI/RHI.h"


#include "Athena/Core/Application.h"

namespace Athena
{
	Vector2u Texture::GetMipSize(uint32 mip) const
	{
		uint32 width = GetWidth();
		uint32 height = GetHeight();

		Vector2u mipSize;
		mipSize.x = Math::Max(float(width) * Math::Pow<float>(0.5f, mip), 1.f);
		mipSize.y = Math::Max(float(height) * Math::Pow<float>(0.5f, mip), 1.f);

		return mipSize;
	}

	uint32 Texture::GetMipLevelsCount() const
	{
		if (!m_Info.GenerateMipMap)
			return 1;

		return Math::Floor(Math::Log2(Math::Max<float>(GetWidth(), GetHeight()))) + 1;
	}

	Ref<TextureView> TextureView::Create(const TextureViewCreateInfo& info, Texture* texture)
	{
		TRACY_PROFILE_FUNC();

		Ref<TextureView> view = RHI::Get()->CreateTextureView(info, texture);

		ENQUEUE_RENDER_COMMAND([view]() 
			{
				view->Init();
			});

		return view;
	}

	TextureView::TextureView(const TextureViewCreateInfo& info, Texture* texture)
		: m_Texture(texture),
		m_Info(info)
	{

	}

	void TextureView::SetSampler(const TextureSamplerCreateInfo& samplerInfo)
	{
		m_Info.Sampler = samplerInfo;
	}

	Ref<Texture> Texture::Create(const TextureCreateInfo& info, Buffer* pixels)
	{
		TRACY_PROFILE_FUNC();

		Ref<Texture> texture = RHI::Get()->CreateTexture(info);

		if (pixels)
		{
			Buffer pixelsCopy = Buffer::Copy(*pixels);

			ENQUEUE_RENDER_COMMAND([texture, pixelsCopyRT = std::move(pixelsCopy)]()
				{
					texture->Init();
					texture->UploadAndBlitMipMap(pixelsCopyRT);
				});
		}
		else
		{
			ENQUEUE_RENDER_COMMAND([texture]()
				{
					texture->Init();
				});
		}

		return texture;
	}

	Texture::Texture(const TextureCreateInfo& info)
		: m_Info(info)
	{

	}

	Texture::~Texture()
	{
		ClearViews();
	}

	void Texture::Resize(uint32 width, uint32 height)
	{
		m_Info.Width = width;
		m_Info.Height = height;
	}

	Ref<TextureView> Texture::GetMainView()
	{
		TextureViewCreateInfo info;
		info.Name = GetName();
		info.ViewType = GetType();
		info.Range.BaseLayer = 0;
		info.Range.LayerCount = GetInfo().Layers;
		info.Range.BaseMipLevel = 0;
		info.Range.MipLevelCount = GetMipLevelsCount();
		info.OverrideSampler = false;

		return GetView(info);
	}

	Ref<TextureView> Texture::GetMipView(uint32 mip)
	{
		TextureViewCreateInfo info;
		info.Name = GetName();
		info.ViewType = GetType();
		info.Range.BaseLayer = 0;
		info.Range.LayerCount = GetInfo().Layers;
		info.Range.BaseMipLevel = mip;
		info.Range.MipLevelCount = 1;
		info.OverrideSampler = false;

		return GetView(info);
	}

	Ref<TextureView> Texture::GetLayersView(uint32 firstLayer, uint32 layerCount)
	{
		TextureType viewType = TextureType::TEXTURE_2D;

		if (GetType() == TextureType::TEXTURE_CUBE && firstLayer % 6 == 0 && layerCount % 6 == 0)
			viewType = TextureType::TEXTURE_CUBE;

		TextureViewCreateInfo info;
		info.Name = GetName();
		info.ViewType = viewType;
		info.Range.BaseLayer = firstLayer;
		info.Range.LayerCount = layerCount;
		info.Range.BaseMipLevel = 0;
		info.Range.MipLevelCount = 1;
		info.OverrideSampler = false;

		return GetView(info);
	}

	Ref<TextureView> Texture::GetView(const TextureViewCreateInfo& info)
	{
		if (m_TextureViews.contains(info))
			return m_TextureViews.at(info);

		m_TextureViews[info] = TextureView::Create(info, this);
		return m_TextureViews.at(info);
	}

	void Texture::InvalidateViews()
	{
		for (auto& [info, view] : m_TextureViews)
		{
			view->Invalidate();
		}
	}

	void Texture::ClearViews()
	{
		m_TextureViews.clear();
	}

	uint32 Texture::GetTotalGPUMemory()
	{
		uint32 totalGPUMemory = 0;

		for (uint32 mip = 0; mip < GetMipLevelsCount(); mip++)
		{
			Vector2u mipSize = GetMipSize(mip);
			uint32 sizeOfMip = mipSize.x * mipSize.y * GetFormatProperties(GetFormat()).BytesPerPixel;
		
			totalGPUMemory += sizeOfMip;
		}

		totalGPUMemory *= GetInfo().Layers;
		return totalGPUMemory;
	}

	Ref<TextureAsset> TextureAsset::s_DefaultTexture;

	TextureAsset::TextureAsset()
		: TextureAsset(EngineRenderAssets::Get().WhiteTexture)
	{

	}

	TextureAsset::TextureAsset(const Ref<Texture>& texture)
	{
		m_Texture = texture;
		SetTexCoords({ Vector2{0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f} });
	}

	TextureAsset::TextureAsset(const Ref<Texture>& texture, const std::array<Vector2, 4>& texCoords)
	{
		m_Texture = texture;
		SetTexCoords(texCoords);
	}

	TextureAsset::TextureAsset(const Ref<Texture>& texture, const Vector2& min, const Vector2& max)
	{
		m_Texture = texture;
		SetTexCoords(min, max);
	}

	TextureAsset::~TextureAsset()
	{
		ENQUEUE_RENDER_COMMAND([texture = m_Texture]() mutable
			{
				texture.Release();
			});
	}

	void TextureAsset::SetTexCoords(const Vector2& min, const Vector2& max)
	{
		float width = (float)m_Texture->GetInfo().Width;
		float height = (float)m_Texture->GetInfo().Height;

		m_TexCoords[0] = { min.x / width, min.y / height };
		m_TexCoords[1] = { max.x / width, min.y / height };
		m_TexCoords[2] = { max.x / width, max.y / height };
		m_TexCoords[3] = { min.x / width, max.y / height };
	}

	bool TextureAsset::Serialize(const FilePath& absolutePath) const
	{
		return true;
	}

	bool TextureAsset::Deserialize(const FilePath& absolutePath, Ref<AssetImportSettings> importSettings)
	{
		TextureImporter importer(importSettings);
		m_Texture = importer.Import(absolutePath);

		if (!m_Texture)
		{
			m_Texture = EngineRenderAssets::Get().WhiteTexture;
			return false;
		}

		return true;
	}
}
