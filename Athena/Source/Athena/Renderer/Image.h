#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"


namespace Athena
{
	enum class ImageFormat
	{
		NONE = 0,
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

	enum class ImageUsage
	{
		SHADER_READ_ONLY,
		ATTACHMENT,
	};

	enum class ImageType
	{
		IMAGE_2D,
		IMAGE_CUBE
	};

	struct ImageCreateInfo
	{
		String Name;
		ImageFormat Format = ImageFormat::RGBA8;
		ImageUsage Usage = ImageUsage::SHADER_READ_ONLY;
		ImageType Type = ImageType::IMAGE_2D;
		const void* InitialData = nullptr;
		uint32 Width = 1;
		uint32 Height = 1;
		uint32 Layers = 1;
		uint32 MipLevels = 1;
	};

	class ATHENA_API Image : public RefCounted
	{
	public:
		static Ref<Image> Create(const ImageCreateInfo& info);
		static Ref<Image> Create(const FilePath& path);
		static Ref<Image> Create(const String& name, const void* data, uint32 width, uint32 height);
		virtual ~Image() = default;

		const ImageCreateInfo& GetInfo() const { return m_Info; }

	public:
		static bool IsDepthFormat(ImageFormat format);
		static bool IsStencilFormat(ImageFormat format);
		static bool IsColorFormat(ImageFormat format);
		static bool IsHDRFormat(ImageFormat format);
		static uint32 BytesPerPixel(ImageFormat format);

	protected:
		ImageCreateInfo m_Info;
	};


	inline bool Image::IsDepthFormat(ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::DEPTH16:		   return true;
		case ImageFormat::DEPTH24STENCIL8: return true;
		case ImageFormat::DEPTH32F:		   return true;
		}

		return false;
	}

	inline bool Image::IsStencilFormat(ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::DEPTH24STENCIL8: return true;
		}

		return false;
	}

	inline bool Image::IsColorFormat(ImageFormat format)
	{
		return !IsDepthFormat(format) && !IsStencilFormat(format);
	}

	inline bool Image::IsHDRFormat(ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::RG16F:		return true;
		case ImageFormat::R11G11B10F:   return true;
		case ImageFormat::RGB16F:		return true;
		case ImageFormat::RGB32F:		return true;
		case ImageFormat::RGBA16F:	    return true;
		case ImageFormat::RGBA32F:	    return true;
		case ImageFormat::DEPTH32F:	    return true;
		}

		return false;
	}

	inline uint32 Image::BytesPerPixel(ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::RGB8:			   return 3 * 1;
		case ImageFormat::RGB8_SRGB:	   return 3 * 1;
		case ImageFormat::RGBA8:		   return 4 * 1;
		case ImageFormat::RGBA8_SRGB:	   return 4 * 1;
		case ImageFormat::RG16F:		   return 2 * 2;
		case ImageFormat::R11G11B10F:	   return 4;
		case ImageFormat::RGB16F:		   return 3 * 2;
		case ImageFormat::RGB32F:		   return 3 * 4;
		case ImageFormat::RGBA16F:		   return 4 * 2;
		case ImageFormat::RGBA32F:		   return 4 * 4;

		case ImageFormat::DEPTH16:		   return 2;
		case ImageFormat::DEPTH24STENCIL8: return 4;
		case ImageFormat::DEPTH32F:		   return 4;
		}

		ATN_CORE_ASSERT(false);
		return false;
	}
}
