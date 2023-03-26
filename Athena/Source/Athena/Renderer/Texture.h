#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Math/Vector.h"

#include "Athena/Renderer/Color.h"

#include <array>


namespace Athena
{
	enum class TextureFormat
	{
		NONE = 0,

		// Color
		RG16F,
		R11F_G11F_B10F,
		RGBA8,
		RGB16F,
		RGB32F,
		RED_INTEGER,

		//Depth/Stencil
		DEPTH24STENCIL8,
		DEPTH32F
	};

	enum class TextureFilter
	{
		LINEAR = 1,
		NEAREST = 2,
		LINEAR_MIPMAP_LINEAR = 3
	};

	enum class TextureWrap
	{
		REPEAT = 1,
		CLAMP_TO_EDGE = 2,
		CLAMP_TO_BORDER = 3,
		MIRRORED_REPEAT = 4,
		MIRRORED_CLAMP_TO_EDGE = 5
	};

	enum class TextureCompareMode
	{
		NONE = 0,
		REF = 1
	};

	enum class TextureCompareFunc
	{
		NONE = 0,
		LEQUAL = 1
	};

	struct TextureSamplerDescription
	{
		TextureFilter MinFilter = TextureFilter::LINEAR;
		TextureFilter MagFilter = TextureFilter::LINEAR;
		TextureWrap Wrap = TextureWrap::REPEAT;

		LinearColor BorderColor = LinearColor::Black;

		TextureCompareMode CompareMode = TextureCompareMode::NONE;
		TextureCompareFunc CompareFunc = TextureCompareFunc::NONE;
	};


	class ATHENA_API Texture
	{
	public:
		virtual ~Texture() = default;

		inline void* GetRendererID() const { return (void*)m_RendererID; };

		virtual void Bind(uint32 slot = 0) const = 0;
		virtual void BindAsImage(uint32 slot = 0, uint32 level = 0) = 0;

		virtual bool IsLoaded() const = 0;

	protected:
		uint64 m_RendererID = 0;
	};


	class ATHENA_API Texture2D: public Texture
	{
	public:
		static Ref<Texture2D> Create(const FilePath& path, 
			bool sRGB = false, const TextureSamplerDescription& samplerDesc = TextureSamplerDescription());

		static Ref<Texture2D> Create(const void* Data, uint32 Width, uint32 Height, 
			bool sRGB = false, const TextureSamplerDescription& samplerDesc = TextureSamplerDescription());

		static Ref<Texture2D> Create(TextureFormat Format, uint32 Width, uint32 Height, 
			const TextureSamplerDescription& samplerDesc = TextureSamplerDescription());

		virtual uint32 GetWidth() const = 0;
		virtual uint32 GetHeight() const = 0;

		virtual void SetData(const void* data, uint32 size) = 0;
		virtual const FilePath& GetFilePath() const = 0;

		bool operator==(const Texture2D& other) const { return GetRendererID() == other.GetRendererID(); }
	};

	class ATHENA_API Texture2DInstance
	{
	public:
		Texture2DInstance();
		Texture2DInstance(const Ref<Texture2D>& texture);
		Texture2DInstance(const Ref<Texture2D>& texture, const std::array<Vector2, 4>& texCoords);
		Texture2DInstance(const Ref<Texture2D>& texture, const Vector2& min, const Vector2& max);

		inline const Ref<Texture2D>& GetNativeTexture() const { return m_Texture; }
		inline const std::array<Vector2, 4>& GetTexCoords() const { return m_TexCoords; };

		inline void SetTexture(const Ref<Texture2D>& texture) { m_Texture = texture; }
		inline void SetTexCoords(const std::array<Vector2, 4>& texCoords) { m_TexCoords = texCoords; }
		void SetTexCoords(const Vector2& min, const Vector2& max);

	private:
		Ref<Texture2D> m_Texture;
		std::array<Vector2, 4> m_TexCoords;
	};


	enum class TextureCubeTarget
	{
		POSITIVE_X = 1,
		NEGATIVE_X = 2,
		POSITIVE_Y = 3,
		NEGATIVE_Y = 4,
		POSITIVE_Z = 5,
		NEGATIVE_Z = 6,
	};

	class ATHENA_API TextureCube: public Texture
	{
	public:
		static Ref<TextureCube> Create(const std::array<std::pair<TextureCubeTarget, FilePath>, 6>& faces, 
			bool sRGB = false, const TextureSamplerDescription& samplerDesc = TextureSamplerDescription());

		static Ref<TextureCube> Create(TextureFormat format, uint32 width, uint32 height, 
			const TextureSamplerDescription& samplerDesc = TextureSamplerDescription());

		virtual void GenerateMipMap(uint32 maxLevel) = 0;
		virtual void SetFilters(TextureFilter min, TextureFilter mag) = 0;
	};

	class ATHENA_API TextureSampler
	{
	public:
		static Ref<TextureSampler> Create(const TextureSamplerDescription& desc);
		
		virtual void Bind(uint32 slot = 0) const = 0;
	};
}
