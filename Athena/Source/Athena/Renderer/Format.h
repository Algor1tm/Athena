#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	enum class Format
	{
		NONE = 0,
		// Color
		R8,
		R8_SRGB,
		RG8,
		RG8_SRGB,
		RGB8,
		RGB8_SRGB,
		RGBA8,
		RGBA8_SRGB,

		R32F,
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

	namespace FormatUtils
	{
		inline bool IsDepthFormat(Format format)
		{
			switch (format)
			{
			case Format::DEPTH16:		 return true;
			case Format::DEPTH24STENCIL8: return true;
			case Format::DEPTH32F:		 return true;
			}

			return false;
		}

		inline bool IsStencilFormat(Format format)
		{
			switch (format)
			{
			case Format::DEPTH24STENCIL8: return true;
			}

			return false;
		}

		inline bool IsColorFormat(Format format)
		{
			return !IsDepthFormat(format) && !IsStencilFormat(format);
		}

		inline bool IsHDRFormat(Format format)
		{
			switch (format)
			{
			case Format::R32F:			return true;
			case Format::RG16F:			return true;
			case Format::R11G11B10F:	return true;
			case Format::RGB16F:		return true;
			case Format::RGB32F:		return true;
			case Format::RGBA16F:		return true;
			case Format::RGBA32F:		return true;
			case Format::DEPTH32F:		return true;
			}

			return false;
		}

		inline uint32 BytesPerPixel(Format format)
		{
			switch (format)
			{
			case Format::R8:			   return 1;
			case Format::R8_SRGB:		   return 1;
			case Format::RG8:			   return 2;
			case Format::RG8_SRGB:		   return 2;
			case Format::RGB8:			   return 3 * 1;
			case Format::RGB8_SRGB:		   return 3 * 1;
			case Format::RGBA8:			   return 4 * 1;
			case Format::RGBA8_SRGB:	   return 4 * 1;

			case Format::R32F:			   return 1 * 4;
			case Format::RG16F:			   return 2 * 2;
			case Format::R11G11B10F:	   return 4;
			case Format::RGB16F:		   return 3 * 2;
			case Format::RGB32F:		   return 3 * 4;
			case Format::RGBA16F:	       return 4 * 2;
			case Format::RGBA32F:	       return 4 * 4;

			case Format::DEPTH16:    	  return 2;
			case Format::DEPTH24STENCIL8: return 4;
			case Format::DEPTH32F:		  return 4;
			}

			ATN_CORE_ASSERT(false);
			return false;
		}

		inline uint32 ChannelsNum(Format format)
		{
			switch (format)
			{
			case Format::R8:
			case Format::R8_SRGB:
			case Format::R32F:
			case Format::DEPTH16:
			case Format::DEPTH32F:
				return 1;
			case Format::RG8:
			case Format::RG8_SRGB:
			case Format::RG16F:
			case Format::DEPTH24STENCIL8:
				return 2;
			case Format::RGB8:
			case Format::RGB8_SRGB:
			case Format::R11G11B10F:
			case Format::RGB16F:
			case Format::RGB32F:
				return 3;
			case Format::RGBA8:
			case Format::RGBA8_SRGB:
			case Format::RGBA16F:
			case Format::RGBA32F:
				return 4;
			}

			ATN_CORE_ASSERT(false);
			return false;
		}
	}
}
