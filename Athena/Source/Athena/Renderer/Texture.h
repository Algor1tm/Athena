#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Math/Vector.h"

#include <array>


namespace Athena
{
	enum class TextureFormat
	{
		NONE = 0,

		// Color
		RG16F,
		RGBA8,
		RGB16F,
		RGB32F,
		RED_INTEGER,

		//Depth/Stencil
		DEPTH24STENCIL8,
		DEPTH32
	};

	enum class TextureTarget
	{
		TEXTURE_2D = 0,
		TEXTURE_2D_MULTISAMPLE = 1,

		TEXTURE_CUBE_MAP_POSITIVE_X = 2,
		TEXTURE_CUBE_MAP_NEGATIVE_X = 3,
		TEXTURE_CUBE_MAP_POSITIVE_Y = 4,
		TEXTURE_CUBE_MAP_NEGATIVE_Y = 5,
		TEXTURE_CUBE_MAP_POSITIVE_Z = 6,
		TEXTURE_CUBE_MAP_NEGATIVE_Z = 7,
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
		MIRRORED_REPEAT = 3,
		MIRRORED_CLAMP_TO_EDGE = 4
	};

	class ATHENA_API Texture
	{
	public:
		virtual ~Texture() = default;

		inline void* GetRendererID() const { return (void*)m_RendererID; };

		virtual void Bind(uint32 slot = 0) const = 0;
		virtual bool IsLoaded() const = 0;

	protected:
		uint64 m_RendererID = 0;
	};


	struct Texture2DDescription
	{
		Texture2DDescription() = default;

		Texture2DDescription(const FilePath& path)
			: TexturePath(path) {}

		Texture2DDescription(const char* path)
			: TexturePath(path) {}

		FilePath TexturePath;

		const void* Data = nullptr;
		uint32 Width = 0;
		uint32 Height = 0;

		TextureFormat Format = TextureFormat::RGBA8;
		bool sRGB = false;

		TextureFilter MinFilter = TextureFilter::LINEAR;
		TextureFilter MagFilter = TextureFilter::LINEAR;
		TextureWrap Wrap = TextureWrap::REPEAT;
	};

	class ATHENA_API Texture2D: public Texture
	{
	public:
		static Ref<Texture2D> Create(const Texture2DDescription& desc);
		static Ref<Texture2D> WhiteTexture();

		virtual uint32 GetWidth() const = 0;
		virtual uint32 GetHeight() const = 0;

		virtual void SetData(const void* data, uint32 size) = 0;
		virtual const FilePath& GetFilePath() const = 0;

		bool operator==(const Texture2D& other) const { return GetRendererID() == other.GetRendererID(); }

	private:
		static Ref<Texture2D> m_WhiteTexture;
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


	struct CubemapDescription
	{
		std::array<std::pair<TextureTarget, FilePath>, 6> Faces;

		uint32 Width = 0;
		uint32 Height = 0;

		TextureFormat Format = TextureFormat::RGBA8;
		bool sRGB = false;

		TextureFilter MinFilter = TextureFilter::LINEAR;
		TextureFilter MagFilter = TextureFilter::LINEAR;
		TextureWrap Wrap = TextureWrap::CLAMP_TO_EDGE;
	};

	class ATHENA_API Cubemap: public Texture
	{
	public:
		static Ref<Cubemap> Create(const CubemapDescription& desc);

		virtual void GenerateMipMap(uint32 count) = 0;
	};
}
