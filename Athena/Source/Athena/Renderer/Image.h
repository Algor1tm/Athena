#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	enum class ImageFormat
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

	enum class ImageUsage
	{
		NONE = 0,
		DEFAULT,
		ATTACHMENT,
	};

	struct ImageCreateInfo
	{
		uint32 Width = 0;
		uint32 Height = 0;
		ImageFormat Format = ImageFormat::RGBA8;
		ImageUsage Usage = ImageUsage::DEFAULT;
	};


	class ATHENA_API Image
	{
	public:
		static Ref<Image> Create(const ImageCreateInfo& info);

		virtual uint32 GetWidth() const = 0;
		virtual uint32 GetHeight() const = 0;

		virtual void* GetDescriptorSet() = 0;

	public:
		static bool IsDepthFormat(ImageFormat format);
		static bool IsStencilFormat(ImageFormat format);
	};


	inline bool Image::IsDepthFormat(ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::DEPTH16: return true;
		case ImageFormat::DEPTH24STENCIL8: return true;
		case ImageFormat::DEPTH32F: return true;
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
}