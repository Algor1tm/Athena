#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Asset/Asset.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Math/Vector.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Format.h"
#include "Athena/Renderer/RenderResource.h"

#include <array>


namespace Athena
{
	enum TextureUsage
	{
		NONE = BIT(0),
		SAMPLED = BIT(1),
		STORAGE = BIT(2),
		ATTACHMENT = BIT(3),

		DEFAULT = SAMPLED
	};

	enum class TextureType
	{
		TEXTURE_2D,
		TEXTURE_CUBE
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

	struct TextureViewCreateInfo
	{
		bool operator==(const TextureViewCreateInfo& other) const = default;

		String Name; // Optional (if empty, it will be generated)

		uint32 BaseMipLevel = 0;
		uint32 MipLevelCount = 1;
		uint32 BaseLayer = 0;
		uint32 LayerCount = 1;

		bool EnableAlphaBlending = true;
		bool GrayScale = false;

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
			string str = std::format("{}{}{}", (uint32)value.Filter, (uint32)value.Wrap, (uint32)value.Compare);
			return hash<string>()(str);
		}
	};

	template<>
	struct hash<TextureViewCreateInfo>
	{
		size_t operator()(const TextureViewCreateInfo& value) const
		{
			size_t hash = value.BaseMipLevel ^ value.MipLevelCount + value.BaseLayer ^ value.LayerCount;
			hash = (hash << 1) + value.EnableAlphaBlending;
			hash = (hash << 1) + value.GrayScale;
			hash = (hash << 1) + value.OverrideSampler;
			return hash;
		}
	};
}

namespace Athena
{
	class Texture;

	class ATHENA_API TextureView : public RenderResource
	{
	public:
		static Ref<TextureView> Create(Texture* texture, const TextureViewCreateInfo& info);
		virtual ~TextureView() = default;

		virtual void Invalidate() = 0;

		virtual const String& GetName() const override { return m_Info.Name; }

		Texture* GetTexture() const { return m_Texture; }
		const TextureViewCreateInfo& GetInfo() const { return m_Info; }

	protected:
		Texture* m_Texture;
		TextureViewCreateInfo m_Info;
	};

	struct TextureCreateInfo
	{
		String Name;
		Format TextureFormat = Format::RGBA8;
		TextureUsage Usage = TextureUsage::DEFAULT;
		uint32 Width = 1;
		uint32 Height = 1;
		uint32 Layers = 1;
		bool GenerateMipMap = false;
		TextureSamplerCreateInfo Sampler;
	};

	class ATHENA_API Texture : public RenderResource
	{
	public:
		virtual ~Texture();

		virtual TextureType GetType() const = 0;

		virtual void Resize(uint32 width, uint32 height) = 0;
		virtual void SetSampler(const TextureSamplerCreateInfo& samplerInfo) = 0;

		virtual void WriteContentToBuffer(Buffer* dstBuffer) = 0;
		virtual uint32 GetImageLayerCount() const = 0;

		Vector2u GetMipSize(uint32 mip) const;
		uint32 GetMipLevelsCount() const;

		Ref<TextureView> GetMipView(uint32 mip);
		Ref<TextureView> GetLayerView(uint32 layer);
		Ref<TextureView> GetView(const TextureViewCreateInfo& info);
		void InvalidateViews();
		void ClearViews();

		uint32 GetTotalGPUMemory();

		uint32 GetWidth() const { return m_Info.Width; }
		uint32 GetHeight() const { return m_Info.Height; }
		Vector2u GetSize() const { return { m_Info.Width, m_Info.Height }; }
		Format GetFormat() const { return m_Info.TextureFormat; }

		const TextureCreateInfo& GetInfo() const { return m_Info; };

	protected:
		TextureCreateInfo m_Info;
		std::unordered_map<TextureViewCreateInfo, Ref<TextureView>> m_TextureViews;
	};


	class ATHENA_API Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(const TextureCreateInfo& info, Buffer data = Buffer());

		virtual TextureType GetType() const override { return TextureType::TEXTURE_2D; }

		virtual RenderResourceType GetResourceType() const override { return RenderResourceType::Texture2D; }
		virtual const String& GetName() const override { return m_Info.Name; }

		virtual uint32 GetImageLayerCount() const override { return m_Info.Layers; }
		const FilePath& GetFilePath() const { return m_FilePath; }

	private:
		friend class TextureImporter;

	private:
		FilePath m_FilePath;
	};

	class ATHENA_API TextureCube: public Texture
	{
	public:
		static Ref<TextureCube> Create(const TextureCreateInfo& info, Buffer data = Buffer());

		virtual TextureType GetType() const override { return TextureType::TEXTURE_CUBE; }

		virtual RenderResourceType GetResourceType() const override { return RenderResourceType::TextureCube; }
		virtual const String& GetName() const override { return m_Info.Name; }

		virtual uint32 GetImageLayerCount() const override { return m_Info.Layers * 6; }
	};


	class ATHENA_API TextureAsset : public Asset
	{
	public:
		TextureAsset();
		TextureAsset(const Ref<Texture2D>& texture);
		TextureAsset(const Ref<Texture2D>& texture, const std::array<Vector2, 4>& texCoords);
		TextureAsset(const Ref<Texture2D>& texture, const Vector2& min, const Vector2& max);

		static Ref<TextureAsset> GetDefault();
		static void Clear();

		virtual AssetType GetAssetType() const override { return AssetType::Texture; }

		virtual bool Serialize(const FilePath& absolutePath) const override;
		virtual bool Deserialize(const FilePath& absolutePath, Ref<AssetImportSettings> importSettings) override;

		const Ref<Texture2D>& GetRenderTexture() const { return m_Texture; }
		const std::array<Vector2, 4>& GetTexCoords() const { return m_TexCoords; };

		void SetTexCoords(const std::array<Vector2, 4>& texCoords) { m_TexCoords = texCoords; }
		void SetTexCoords(const Vector2& min, const Vector2& max);

	private:
		Ref<Texture2D> m_Texture;
		std::array<Vector2, 4> m_TexCoords;

		static Ref<TextureAsset> s_DefaultTexture;
	};

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

			ATN_CORE_ASSERT(false);
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

			ATN_CORE_ASSERT(false);
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

			ATN_CORE_ASSERT(false);
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

			ATN_CORE_ASSERT(false);
			return TextureWrap::REPEAT;
		}
	}
}
