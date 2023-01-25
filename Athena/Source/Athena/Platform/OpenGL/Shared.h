#pragma once

#include "Athena/Renderer/Texture.h"
#include <glad/glad.h>


namespace Athena
{
	inline GLenum TextureFormatToGLenum(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RGBA8: return GL_RGBA8;
		case TextureFormat::RGB16F: return GL_RGB16F;
		case TextureFormat::RED_INTEGER: return GL_RED_INTEGER;
		case TextureFormat::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8;
		case TextureFormat::DEPTH32: return GL_DEPTH_COMPONENT32;
		}

		ATN_CORE_ASSERT(false, "Unknown texture format!");
		return GL_NONE;
	}

	inline GLenum GLTextureTarget(TextureTarget target)
	{
		switch (target)
		{
		case TextureTarget::TEXTURE_2D: return GL_TEXTURE_2D;
		case TextureTarget::TEXTURE_2D_MULTISAMPLE: return GL_TEXTURE_2D_MULTISAMPLE;

		case TextureTarget::TEXTURE_CUBE_MAP_POSITIVE_X: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		case TextureTarget::TEXTURE_CUBE_MAP_NEGATIVE_X: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
		case TextureTarget::TEXTURE_CUBE_MAP_POSITIVE_Y: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
		case TextureTarget::TEXTURE_CUBE_MAP_NEGATIVE_Y: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
		case TextureTarget::TEXTURE_CUBE_MAP_POSITIVE_Z: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
		case TextureTarget::TEXTURE_CUBE_MAP_NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
		}

		ATN_CORE_ASSERT(false, "Unknown texture target!");
		return GL_NONE;
	}

	inline GLenum AthenaTextureWrapToGLenum(TextureWrap wrap)
	{
		switch (wrap)
		{
		case TextureWrap::REPEAT: return GL_REPEAT;
		case TextureWrap::CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
		case TextureWrap::MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
		case TextureWrap::MIRRORED_CLAMP_TO_EDGE: return GL_MIRROR_CLAMP_TO_EDGE;
		}

		ATN_CORE_ASSERT(false, "Unknown texture wrap!");
		return GL_NONE;
	}

	inline GLenum AthenaTextureFilterToGLenum(TextureFilter filter)
	{
		switch (filter)
		{
		case TextureFilter::LINEAR: return GL_LINEAR;
		case TextureFilter::NEAREST: return GL_NEAREST;
		case TextureFilter::LINEAR_MIPMAP_LINEAR: return GL_LINEAR_MIPMAP_LINEAR;
		}

		ATN_CORE_ASSERT(false, "Unknown texture filter!");
		return GL_NONE;
	}

	inline void GetGLFormatAndType(TextureFormat format, GLenum& internalFormat, GLenum& dataFormat, GLenum& type)
	{
		switch (format)
		{
		case TextureFormat::RGB16F:
		{
			internalFormat = GL_RGB16F;
			dataFormat = GL_RGB;
			type = GL_FLOAT;
			break;
		}
		case TextureFormat::RGBA8:
		{
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
			type = GL_UNSIGNED_BYTE;
			break;
		}
		default:
		{
			ATN_CORE_ASSERT(false, "Invalid texture format!");
		}
		}
	}

	inline bool GetGLFormats(int channels, bool HDR, bool sRGB, GLenum& internalFormat, GLenum& dataFormat)
	{
		if ((HDR && sRGB))
			return false;

		if (channels == 1)
		{
			internalFormat = HDR ? GL_R16F : GL_R8;
			dataFormat = GL_RED;
		}
		else if (channels == 3)
		{
			if (HDR)
			{
				internalFormat = GL_RGB16F;
			}
			else
			{
				if (sRGB)
					internalFormat = GL_SRGB8;
				else
					internalFormat = GL_RGB8;
			}

			dataFormat = GL_RGB;
		}
		else if (channels == 4)
		{
			if (HDR)
			{
				internalFormat = GL_RGBA16F;
			}
			else
			{
				if (sRGB)
					internalFormat = GL_SRGB8_ALPHA8;
				else
					internalFormat = GL_RGBA8;
			}

			dataFormat = GL_RGBA;
		}
		else
		{
			return false;
		}

		return true;
	}
}

