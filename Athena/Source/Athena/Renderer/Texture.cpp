#include "Texture.h"

#include "Athena/Asset/Editor/TextureImporter.h"
#include "Athena/Renderer/RenderThread.h"
#include "Athena/Renderer/EngineRenderAssets.h"
#include "Athena/Renderer/RHI/RHI.h"


#include "Athena/Core/Application.h"

namespace Athena
{
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

					Ref<CommandContext> cmdContext = RHI::Get()->GetContext(CommandContextType::AsyncTransfer);

					cmdContext->Transition(texture.Raw(), ResourceState::CopyDst);
					cmdContext->UploadTexture(texture.Raw(), pixelsCopyRT);

					// TODO Generate MipMap

					cmdContext->TransitionOwnership(texture.Raw(), CommandContextType::Graphics, ResourceState::SRVAll);
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

	static uint32 ComputeMipCount(const TextureCreateInfo& info)
	{
		if (!info.GenerateMipMap)
			return 1;

		return 1;
		//return Math::Floor(Math::Log2(Math::Max<float>(info.Width, info.Height))) + 1;
	}

	Texture::Texture(const TextureCreateInfo& info)
		: m_Info(info)
		, m_MipCount(ComputeMipCount(info))
		, m_PlaneCount(GetFormatProperties(info.TextureFormat).NumPlanes)
		, m_MainState(ResourceState::Init)
		, m_StatesCollapsed(true)
	{
		m_SubresourcesStates.resize(GetMipCount() * GetLayerCount() * GetPlaneCount(), ResourceState::Init);
	}

	Texture::~Texture()
	{
		ClearViews();
	}

	void Texture::Resize(uint32 width, uint32 height)
	{
		m_Info.Width = width;
		m_Info.Height = height;
		m_MipCount = ComputeMipCount(m_Info);
	}

	Ref<TextureView> Texture::GetView(TextureViewType viewType)
	{
		TextureViewCreateInfo info;
		info.Name = GetName();
		info.ViewType = viewType;
		info.TextureType = GetType();
		info.Range.BaseLayer = 0;
		info.Range.LayerCount = GetInfo().Layers;
		info.Range.BaseMipLevel = 0;
		info.Range.MipLevelCount = GetMipCount();
		info.OverrideSampler = false;

		return GetView(info);
	}

	Ref<TextureView> Texture::GetMipView(uint32 mip, TextureViewType viewType)
	{
		TextureViewCreateInfo info;
		info.Name = GetName();
		info.ViewType = viewType;
		info.TextureType = GetType();
		info.Range.BaseLayer = 0;
		info.Range.LayerCount = GetInfo().Layers;
		info.Range.BaseMipLevel = mip;
		info.Range.MipLevelCount = 1;
		info.OverrideSampler = false;

		return GetView(info);
	}

	Ref<TextureView> Texture::GetLayerView(uint32 layer, TextureViewType viewType)
	{
		TextureViewCreateInfo info;
		info.Name = GetName();
		info.ViewType = viewType;
		info.TextureType = GetType();
		info.Range.BaseLayer = layer;
		info.Range.LayerCount = 1;
		info.Range.BaseMipLevel = 0;
		info.Range.MipLevelCount = GetMipCount();
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

	uint32 Texture::GetSubresourceIndex(const TextureSubresource& subres) const
	{
		uint32 planeIndex = (uint32)subres.Plane;
		ensuref(subres.Plane != TexturePlane::ALL);

		return GetMipCount() * GetLayerCount() * planeIndex + GetMipCount() * subres.Layer + subres.MipLevel;
	}

	ResourceState Texture::GetSubresState(const TextureSubresource& subres) const
	{
		if (m_StatesCollapsed)
			return m_MainState;

		return m_SubresourcesStates[GetSubresourceIndex(subres)];
	}

	void Texture::SetSubresState(ResourceState state, const TextureSubresourceRange& subres)
	{
		uint32 firstPlane = subres.GetFirstPlane();
		uint32 lastPlane = subres.GetLastPlane(GetPlaneCount());

		for (uint32 plane = firstPlane; plane < lastPlane; ++plane)
		{
			for (uint32 layer = subres.BaseLayer; layer < subres.LayerCount; ++layer)
			{
				for (uint32 mip = subres.BaseMipLevel; mip < subres.MipLevelCount; ++mip)
				{
					m_SubresourcesStates[GetSubresourceIndex(TextureSubresource(mip, layer, TexturePlane(plane)))] = state;
				}
			}
		}

		CollapseSubresStatesIfSame();
	}

	bool Texture::AreSubresourcesSameState(const TextureSubresourceRange& subres) const
	{
		return AreSubresourcesSameState(subres, m_SubresourcesStates[0]);
	}

	bool Texture::AreSubresourcesSameState(const TextureSubresourceRange& subres, ResourceState state) const
	{
		uint32 firstPlane = subres.GetFirstPlane();
		uint32 lastPlane = subres.GetLastPlane(GetPlaneCount());

		for (uint32 plane = firstPlane; plane < lastPlane; ++plane)
		{
			for (uint32 layer = subres.BaseLayer; layer < subres.LayerCount; ++layer)
			{
				for (uint32 mip = subres.BaseMipLevel; mip < subres.MipLevelCount; ++mip)
				{
					if (m_SubresourcesStates[GetSubresourceIndex(TextureSubresource(mip, layer, TexturePlane(plane)))] != state)
					{
						return false;
					}
				}
			}
		}

		return true;
	}

	void Texture::CollapseSubresStatesIfSame()
	{
		if (AreSubresourcesSameState(GetFullSubresourceRange()))
		{
			m_StatesCollapsed = true;
			m_MainState = m_SubresourcesStates[0];
		}

		m_StatesCollapsed = false;
	}

	Vector2u Texture::GetMipSize(uint32 mip) const
	{
		if (mip >= GetMipCount())
			return { 0, 0 };

		return { Math::Max(GetWidth() >> mip, 1u), Math::Max(GetHeight() >> mip, 1u) };
	}

	uint32 Texture::GetTotalGPUMemory()
	{
		uint32 totalGPUMemory = 0;

		for (uint32 mip = 0; mip < GetMipCount(); mip++)
		{
			Vector2u mipSize = GetMipSize(mip);
			uint32 sizeOfMip = mipSize.x * mipSize.y * GetFormatProperties(GetFormat()).BytesPerPixel;
		
			totalGPUMemory += sizeOfMip;
		}

		totalGPUMemory *= GetInfo().Layers;
		return totalGPUMemory;
	}

	TextureSubresourceRange Texture::GetFullSubresourceRange() const
	{
		TextureSubresourceRange defaultRange;
		defaultRange.Plane = TexturePlane::ALL;
		defaultRange.BaseLayer = 0;
		defaultRange.LayerCount = m_Info.Layers;
		defaultRange.BaseMipLevel = 0;
		defaultRange.MipLevelCount = GetMipCount();

		return defaultRange;
	}

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
