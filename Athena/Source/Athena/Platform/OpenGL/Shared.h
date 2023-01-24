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

	inline bool GetGLFormats(int channels, bool HDR, GLenum& internalFormat, GLenum& dataFormat)
	{
		if (channels == 1)
		{
			internalFormat = HDR ? GL_R16F : GL_R8;
			dataFormat = GL_RED;
		}
		else if (channels == 3)
		{
			internalFormat = HDR ? GL_RGB16F : GL_RGB8;
			dataFormat = GL_RGB;
		}
		else if (channels == 4)
		{
			internalFormat = HDR ? GL_RGBA16F : GL_RGBA8;
			dataFormat = GL_RGBA;
		}
		else
		{
			return false;
		}

		return true;
	}
}

