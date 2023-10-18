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
		RGB8,
		RGBA8,

		RG16F,
		RGB16F,
		RGB32F,
		RGBA16F,
		RGBA32F,

		//Depth/Stencil
		DEPTH16,
		DEPTH24STENCIL8,
		DEPTH32F
	};

	enum class TextureUsage
	{
		NONE = 0,
		ATTACHMENT,
	};

	enum class TextureFilter
	{
		LINEAR = 1,
		NEAREST = 2,
	};

	enum class TextureWrap
	{
		REPEAT = 1,
		CLAMP_TO_EDGE = 2,
		CLAMP_TO_BORDER = 3,
		MIRRORED_REPEAT = 4,
		MIRRORED_CLAMP_TO_EDGE = 5
	};

	struct TextureSamplerCreateInfo
	{
		TextureFilter MinFilter = TextureFilter::LINEAR;
		TextureFilter MagFilter = TextureFilter::LINEAR;
		TextureFilter MipMapFilter = TextureFilter::LINEAR;

		TextureWrap Wrap = TextureWrap::REPEAT;
	};

	struct TextureCreateInfo
	{
		const void* Data = nullptr;
		uint32 Width = 0;
		uint32 Height = 0;
		bool sRGB = false;
		TextureFormat Format = TextureFormat::RGBA8;
		TextureUsage Usage = TextureUsage::NONE;
		TextureSamplerCreateInfo SamplerInfo;
		bool GenerateMipMap = false;
		bool Layers = 1;
	};


	class ATHENA_API Texture
	{
	public:
		virtual ~Texture() = default;

		virtual void GenerateMipMap(uint32 maxLevel) = 0;
		virtual void ResetSampler(const TextureSamplerCreateInfo& samplerInfo) = 0;

		virtual void* GetDescriptorSet() = 0;

		const TextureCreateInfo& GetInfo() const { return m_Info; };
		
	public:
		static bool IsDepthFormat(TextureFormat format);
		static bool IsStencilFormat(TextureFormat format);
		static bool IsColorFormat(TextureFormat format);
		static bool IsHDRFormat(TextureFormat format);

	protected:
		TextureCreateInfo m_Info;
	};


	class ATHENA_API Texture2D: public Texture
	{
	public:
		static Ref<Texture2D> Create(const TextureCreateInfo& info);				  // Default
		static Ref<Texture2D> Create(const FilePath& path);							  // From file
		static Ref<Texture2D> Create(const void* data, uint32 width, uint32 height);  // From memory

		const FilePath& GetFilePath() const { return m_FilePath; }

	public:
		FilePath m_FilePath;
	};


	class ATHENA_API TextureCube: public Texture
	{
	public:
		static Ref<TextureCube> Create(const TextureCreateInfo& info);
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


	inline bool Texture::IsDepthFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::DEPTH16: return true;
		case TextureFormat::DEPTH24STENCIL8: return true;
		case TextureFormat::DEPTH32F: return true;
		}

		return false;
	}

	inline bool Texture::IsStencilFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::DEPTH24STENCIL8: return true;
		}

		return false;
	}

	inline bool Texture::IsColorFormat(TextureFormat format)
	{
		return !IsDepthFormat(format) && !IsStencilFormat(format);
	}

	inline bool Texture::IsHDRFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RG16F: return true;
		case TextureFormat::RGB16F: return true;
		case TextureFormat::RGB32F: return true;
		case TextureFormat::RGBA16F: return true;
		case TextureFormat::RGBA32F: return true;
		}

		return false;
	}
}
