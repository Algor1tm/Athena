#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Asset/Asset.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Core/Hash.h"
#include "Athena/Math/Vector.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Format.h"

#include <array>


namespace Athena
{
	enum TextureUsage
	{
		NONE = BIT(0),
		SAMPLED = BIT(1),
		UAV = BIT(2),
		ATTACHMENT = BIT(3),

		DEFAULT = SAMPLED
	};

	enum class TextureType
	{
		TEXTURE_2D,
		TEXTURE_CUBE
	};

	enum class TextureViewType
	{
		SRV,
		UAV,
		RTV,
		DSV,
		DSV_READONLY
	};

	enum class ComponentMapping
	{
		IDENTITY = 0,
		ZERO,
		ONE,
		R,
		G,
		B,
		A,
	};

	enum class TextureAspect
	{
		AUTO = 0,
		COLOR,
		DEPTH,
		STENCIL,
		DEPTH_STENCIL
	};

	enum class TextureFilter
	{
		NEAREST = 1,
		LINEAR = 2,
		TRILINEAR = 3
	};

	enum class TextureWrap
	{
		REPEAT = 1,
		CLAMP_TO_EDGE = 2,
		CLAMP_TO_BORDER = 3,
		MIRRORED_REPEAT = 4,
		MIRRORED_CLAMP_TO_EDGE = 5
	};

	enum class TextureCompareOperator
	{
		NONE = 0,
		LESS_OR_EQUAL,
		GREATER_OR_EQUAL
	};

	struct TextureSamplerCreateInfo
	{
		bool operator==(const TextureSamplerCreateInfo& other) const = default;

		TextureFilter Filter = TextureFilter::LINEAR;
		TextureWrap Wrap = TextureWrap::REPEAT;
		float AnisotropyLevel = 0.f;
		TextureCompareOperator Compare = TextureCompareOperator::NONE;
	};

	struct TextureRange
	{
		bool operator==(const TextureRange&) const = default;

		uint32 BaseMipLevel = 0;
		uint32 MipLevelCount = 1;
		uint32 BaseLayer = 0;
		uint32 LayerCount = 1;
		TextureAspect Aspect = TextureAspect::AUTO;
	};

	struct ComponentsSwizzle
	{
		bool operator==(const ComponentsSwizzle&) const = default;

		ComponentMapping R = ComponentMapping::IDENTITY;
		ComponentMapping G = ComponentMapping::IDENTITY;
		ComponentMapping B = ComponentMapping::IDENTITY;
		ComponentMapping A = ComponentMapping::IDENTITY;
	};

	struct TextureViewCreateInfo
	{
		bool operator==(const TextureViewCreateInfo&) const = default;

		String Name; // Optional (if empty, it will be generated)
		TextureType TextureType = TextureType::TEXTURE_2D;
		TextureViewType ViewType = TextureViewType::SRV;
		TextureRange Range;
		ComponentsSwizzle Swizzle;
		bool OverrideSampler = false;
		TextureSamplerCreateInfo Sampler;
	};
}

// Hash functions
namespace std
{
	using namespace Athena;

	template<>
	struct hash<TextureSamplerCreateInfo>
	{
		size_t operator()(const TextureSamplerCreateInfo& value) const
		{
			size_t hash = 0;
			HashCombine(hash, (uint32)value.Filter);
			HashCombine(hash, (uint32)value.Wrap);
			HashCombine(hash, (uint32)value.Compare);

			return hash;
		}
	};

	template<>
	struct hash<TextureViewCreateInfo>
	{
		size_t operator()(const TextureViewCreateInfo& value) const
		{
			size_t hash = 0;
			HashCombine(hash, (uint32)value.ViewType);
			HashCombine(hash, value.Range.BaseMipLevel);
			HashCombine(hash, value.Range.MipLevelCount);
			HashCombine(hash, value.Range.BaseLayer);
			HashCombine(hash, value.Range.LayerCount);
			HashCombine(hash, value.OverrideSampler);

			return hash;
		}
	};
}

namespace Athena
{
	class Texture;

	class ATHENA_API TextureView: public RefCounted
	{
	public:
		static Ref<TextureView> Create(const TextureViewCreateInfo& info, Texture* texture);

		TextureView(const TextureViewCreateInfo& info, Texture* texture);
		virtual ~TextureView() = default;

		virtual void Init() = 0;
		virtual void Invalidate() = 0;

		TextureViewType GetViewType() const { return m_Info.ViewType; }
		TextureType GetTextureType() const { return m_Info.TextureType; }
		const TextureRange& GetRange() const { return m_Info.Range; }

		Texture* GetTexture() const { return m_Texture; }
		const TextureViewCreateInfo& GetInfo() const { return m_Info; }

		void SetSampler(const TextureSamplerCreateInfo& samplerInfo);

	private:
		Texture* m_Texture;
		TextureViewCreateInfo m_Info;
	};

	using TextureView2D = TextureView;
	using TextureViewCube = TextureView;

	struct TextureRegion
	{
		Vector2u Offset = { 0, 0 };
		Vector2u Size = { 0, 0 };
		uint32 Mip = 0;
		uint32 Layer = 0;
	};

	struct TextureCreateInfo
	{
		String Name;
		TextureType Type = TextureType::TEXTURE_2D;
		Format TextureFormat = Format::RGBA8;
		TextureUsage Usage = TextureUsage::DEFAULT;
		uint32 Width = 1;
		uint32 Height = 1;
		uint32 Layers = 1;
		uint32 Samples = 1;
		bool GenerateMipMap = false;
		TextureSamplerCreateInfo Sampler;
	};

	class ATHENA_API Texture: public RefCounted
	{
	public:
		static Ref<Texture> Create(const TextureCreateInfo& info, Buffer* pixels = nullptr);

		Texture(const TextureCreateInfo& info);
		virtual ~Texture();

		virtual void Init() = 0;
		virtual void Resize(uint32 width, uint32 height);
		virtual void Upload(const Buffer& srcBuffer, const TextureRegion& region) = 0;
		virtual void UploadAndBlitMipMap(const Buffer& srcBuffer) = 0;

	public:
		Ref<TextureView> GetView(TextureViewType viewType = TextureViewType::SRV);
		Ref<TextureView> GetMipView(uint32 mip, TextureViewType viewType = TextureViewType::SRV);
		Ref<TextureView> GetLayerView(uint32 layer, TextureViewType viewType = TextureViewType::SRV);
		Ref<TextureView> GetView(const TextureViewCreateInfo& info);
		void InvalidateViews();
		void ClearViews();

		Vector2u GetMipSize(uint32 mip) const;
		uint32 GetMipLevelsCount() const;

		uint32 GetTotalGPUMemory();

		const String& GetName() const { return m_Info.Name; }
		TextureType GetType() const { return m_Info.Type; };
		uint32 GetWidth() const { return m_Info.Width; }
		uint32 GetHeight() const { return m_Info.Height; }
		Vector2u GetSize() const { return { m_Info.Width, m_Info.Height }; }
		Format GetFormat() const { return m_Info.TextureFormat; }

		const TextureCreateInfo& GetInfo() const { return m_Info; };

	private:
		TextureCreateInfo m_Info;
		std::unordered_map<TextureViewCreateInfo, Ref<TextureView>> m_TextureViews;
	};

	using Texture2D = Texture;
	using TextureCube = Texture;


	class ATHENA_API TextureAsset : public Asset
	{
	public:
		TextureAsset();
		TextureAsset(const Ref<Texture2D>& texture);
		TextureAsset(const Ref<Texture2D>& texture, const std::array<Vector2, 4>& texCoords);
		TextureAsset(const Ref<Texture2D>& texture, const Vector2& min, const Vector2& max);
		~TextureAsset();

		virtual AssetType GetAssetType() const override { return AssetType::Texture; }

		virtual bool Serialize(const FilePath& absolutePath) const override;
		virtual bool Deserialize(const FilePath& absolutePath, Ref<AssetImportSettings> importSettings) override;

		const Ref<Texture2D>& GetTexture() const { return m_Texture; }
		const std::array<Vector2, 4>& GetTexCoords() const { return m_TexCoords; };

		void SetTexCoords(const std::array<Vector2, 4>& texCoords) { m_TexCoords = texCoords; }
		void SetTexCoords(const Vector2& min, const Vector2& max);

	private:
		Ref<Texture2D> m_Texture;
		std::array<Vector2, 4> m_TexCoords;

		static Ref<TextureAsset> s_DefaultTexture;
	};

	inline TextureAspect GetAspectFromFormat(Format format)
	{
		auto& properties = GetFormatProperties(format);

		if (properties.HasColor)
			return TextureAspect::COLOR;

		if (properties.HasDepth && properties.HasStencil)
			return TextureAspect::DEPTH_STENCIL;

		if (properties.HasDepth)
			return TextureAspect::DEPTH;

		if (properties.HasStencil)
			return TextureAspect::STENCIL;

		checkf(false);
		return TextureAspect::COLOR;
	}

	namespace EnumUtils
	{
		inline std::string_view TextureFilterToString(TextureFilter filter)
		{
			switch (filter)
			{
				case TextureFilter::NEAREST:   return "NEAREST";
				case TextureFilter::LINEAR:    return "LINEAR";
				case TextureFilter::TRILINEAR: return "TRILINEAR";
			}

			checkf(false);
			return "";
		}

		inline TextureFilter TextureFilterFromString(const String& filterString)
		{
			if (filterString == "NEAREST")
				return TextureFilter::NEAREST;
			else if (filterString == "LINEAR")
				return TextureFilter::LINEAR;
			else if (filterString == "TRILINEAR")
				return TextureFilter::TRILINEAR;

			checkf(false);
			return TextureFilter::NEAREST;
		}

		inline std::string_view TextureWrapToString(TextureWrap wrap)
		{
			switch (wrap)
			{
			case TextureWrap::REPEAT:				  return "REPEAT";
			case TextureWrap::CLAMP_TO_EDGE:		  return "CLAMP_TO_EDGE";
			case TextureWrap::CLAMP_TO_BORDER:		  return "CLAMP_TO_BORDER";
			case TextureWrap::MIRRORED_REPEAT:		  return "MIRRORED_REPEAT";
			case TextureWrap::MIRRORED_CLAMP_TO_EDGE: return "MIRRORED_CLAMP_TO_EDGE";
			}

			checkf(false);
			return "";
		}

		inline TextureWrap TextureWrapFromString(const String& wrapString)
		{
			if (wrapString == "REPEAT")
				return TextureWrap::REPEAT;
			else if (wrapString == "CLAMP_TO_EDGE")
				return TextureWrap::CLAMP_TO_EDGE;
			else if (wrapString == "CLAMP_TO_BORDER")
				return TextureWrap::CLAMP_TO_BORDER;
			else if (wrapString == "MIRRORED_REPEAT")
				return TextureWrap::MIRRORED_REPEAT;
			else if (wrapString == "MIRRORED_CLAMP_TO_EDGE")
				return TextureWrap::MIRRORED_CLAMP_TO_EDGE;

			checkf(false);
			return TextureWrap::REPEAT;
		}
	}
}
