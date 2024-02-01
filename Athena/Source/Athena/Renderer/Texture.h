#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/ShaderResource.h"

#include <array>


namespace Athena
{
	enum class TextureFormat
	{
		NONE,
		// Color
		RGB8,
		RGB8_SRGB,
		RGBA8,
		RGBA8_SRGB,

		RG16F,
		RGB16F,
		R11G11B10F,
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
		SHADER_READ_ONLY,
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
		// TODO: Anisotropy
	};

	struct TextureCreateInfo
	{
		String Name;
		const void* Data = nullptr;
		uint32 Width = 1;
		uint32 Height = 1;
		uint32 Layers = 1;
		uint32 MipLevels = 1;
		bool GenerateMipMap = false;
		TextureFormat Format = TextureFormat::RGBA8;
		TextureUsage Usage = TextureUsage::SHADER_READ_ONLY;
		bool GenerateSampler = true;
		TextureSamplerCreateInfo SamplerInfo;
	};


	class ATHENA_API Texture : public ShaderResource
	{
	public:
		virtual ~Texture() = default;

		virtual void GenerateMipMap(uint32 levels) = 0;
		virtual void SetSampler(const TextureSamplerCreateInfo& samplerInfo) = 0;

		virtual ShaderResourceType GetResourceType() override { return ShaderResourceType::Texture2D; }
		const TextureCreateInfo& GetInfo() const { return m_Info; };
		
	public:
		static bool IsDepthFormat(TextureFormat format);
		static bool IsStencilFormat(TextureFormat format);
		static bool IsColorFormat(TextureFormat format);
		static bool IsHDRFormat(TextureFormat format);
		static uint32 BytesPerPixel(TextureFormat format);

	protected:
		TextureCreateInfo m_Info;
	};


	class ATHENA_API Texture2D: public Texture
	{
	public:
		static Ref<Texture2D> Create(const TextureCreateInfo& info);				  // Default
		static Ref<Texture2D> Create(const FilePath& path);							  // From file
		static Ref<Texture2D> Create(const String& name, const void* data, uint32 width, uint32 height);  // From memory

		virtual void Resize(uint32 width, uint32 height) = 0;

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
		case TextureFormat::DEPTH16:		 return true;
		case TextureFormat::DEPTH24STENCIL8: return true;
		case TextureFormat::DEPTH32F:		 return true;
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
		case TextureFormat::RG16F:		return true;
		case TextureFormat::R11G11B10F: return true;
		case TextureFormat::RGB16F:		return true;
		case TextureFormat::RGB32F:		return true;
		case TextureFormat::RGBA16F:	return true;
		case TextureFormat::RGBA32F:	return true;
		case TextureFormat::DEPTH32F:	return true;
		}

		return false;
	}

	inline uint32 Texture::BytesPerPixel(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RGB8:			   return 3 * 1;
		case TextureFormat::RGB8_SRGB:		   return 3 * 1;
		case TextureFormat::RGBA8:			   return 4 * 1;
		case TextureFormat::RGBA8_SRGB:		   return 4 * 1;
		case TextureFormat::RG16F:			   return 2 * 2;
		case TextureFormat::R11G11B10F:		   return 4;
		case TextureFormat::RGB16F:			   return 3 * 2;
		case TextureFormat::RGB32F:			   return 3 * 4;
		case TextureFormat::RGBA16F:		   return 4 * 2;
		case TextureFormat::RGBA32F:		   return 4 * 4;
			// TODO: Verify
		case TextureFormat::DEPTH16:		   return 2;	
		case TextureFormat::DEPTH24STENCIL8:   return 4;
		case TextureFormat::DEPTH32F:		   return 4;
		}

		ATN_CORE_ASSERT(false);
		return false;
	}
}
