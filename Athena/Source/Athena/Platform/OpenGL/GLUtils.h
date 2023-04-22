#pragma once

#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/RendererAPI.h"
#include "Athena/Renderer/Shader.h"

#include <glad/glad.h>


namespace Athena::Utils
{
	typedef unsigned int GLenum;


	inline GLenum TextureFormatToGLenum(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RGB8: return GL_RGBA8;
		case TextureFormat::RGBA8: return GL_RGBA8;

		case TextureFormat::RG16F: return GL_RG16F;
		case TextureFormat::R11F_G11F_B10F: return GL_R11F_G11F_B10F;
		case TextureFormat::RGB16F: return GL_RGB16F;
		case TextureFormat::RGB32F: return GL_RGB32F;
		case TextureFormat::RGBA16F: return GL_RGBA16F;
		case TextureFormat::RGBA32F: return GL_RGBA32F;

		case TextureFormat::RED_INTEGER: return GL_RED_INTEGER;

		case TextureFormat::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8;
		case TextureFormat::DEPTH32F: return GL_DEPTH_COMPONENT32;
		}

		ATN_CORE_ASSERT(false, "Unknown texture format!");
		return GL_NONE;
	}

	inline GLenum TextureWrapToGLenum(TextureWrap wrap)
	{
		switch (wrap)
		{
		case TextureWrap::REPEAT: return GL_REPEAT;
		case TextureWrap::CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
		case TextureWrap::CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
		case TextureWrap::MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
		case TextureWrap::MIRRORED_CLAMP_TO_EDGE: return GL_MIRROR_CLAMP_TO_EDGE;
		}

		ATN_CORE_ASSERT(false, "Unknown texture wrap!");
		return GL_NONE;
	}

	inline GLenum TextureFilterToGLenum(TextureFilter filter)
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

	inline GLenum TextureCompareModeToGLenum(TextureCompareMode mode)
	{
		switch (mode)
		{
		case TextureCompareMode::REF: return GL_COMPARE_REF_TO_TEXTURE;
		}

		ATN_CORE_ASSERT(false, "Unknown texture compare mode!");
		return GL_NONE;
	}

	inline GLenum TextureCompareFuncToGLenum(TextureCompareFunc func)
	{
		switch (func)
		{
		case TextureCompareFunc::LEQUAL: return GL_LEQUAL;
		}

		ATN_CORE_ASSERT(false, "Unknown texture compare func!");
		return GL_NONE;
	}

	inline void TextureFormatToGLenum(TextureFormat format, GLenum& internalFormat, GLenum& dataFormat, GLenum& type)
	{
		switch (format)
		{
		case TextureFormat::RGB8:
		{
			internalFormat = GL_RGB8;
			dataFormat = GL_RGB;
			type = GL_UNSIGNED_BYTE;
			break;
		}
		case TextureFormat::RGBA8:
		{
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
			type = GL_UNSIGNED_BYTE;
			break;
		}
		case TextureFormat::RG16F:
		{
			internalFormat = GL_RG16F;
			dataFormat = GL_RG;
			type = GL_FLOAT;
			break;
		}
		case TextureFormat::R11F_G11F_B10F:
		{
			internalFormat = GL_R11F_G11F_B10F;
			dataFormat = GL_RGB;
			type = GL_FLOAT;
			break;
		}
		case TextureFormat::RGB16F:
		{
			internalFormat = GL_RGB16F;
			dataFormat = GL_RGB;
			type = GL_FLOAT;
			break;
		}
		case TextureFormat::RGB32F:
		{
			internalFormat = GL_RGB32F;
			dataFormat = GL_RGB;
			type = GL_FLOAT;
			break;
		}
		case TextureFormat::RGBA16F:
		{
			internalFormat = GL_RGBA16F;
			dataFormat = GL_RGBA;
			type = GL_FLOAT;
			break;
		}
		case TextureFormat::RGBA32F:
		{
			internalFormat = GL_RGBA32F;
			dataFormat = GL_RGBA;
			type = GL_FLOAT;
			break;
		}
		case TextureFormat::RED_INTEGER:
		{
			internalFormat = GL_R32I;
			dataFormat = GL_RED_INTEGER;
			type = GL_UNSIGNED_BYTE;
			break;
		}
		case TextureFormat::DEPTH24STENCIL8:
		{
			internalFormat = GL_DEPTH24_STENCIL8;
			dataFormat = GL_DEPTH_STENCIL;
			type = GL_UNSIGNED_BYTE;
			break;
		}
		case TextureFormat::DEPTH32F:
		{
			internalFormat = GL_DEPTH_COMPONENT32F;
			dataFormat = GL_DEPTH;
			type = GL_FLOAT;
			break;
		}
		default: ATN_CORE_ASSERT(false, "Invalid texture format!");
		}

	}

	inline bool GetGLFormats(int channels, bool HDR, bool sRGB, GLenum& internalFormat, GLenum& dataFormat)
	{
		if ((HDR && sRGB) || (HDR && channels == 4))
			return false;

		if (channels == 1)
		{
			internalFormat = HDR ? GL_R16F : GL_R8;
			dataFormat = GL_RED;
		}
		else if (channels == 3)
		{
			if (HDR)
				internalFormat = GL_R11F_G11F_B10F;
			else
				internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;

			dataFormat = GL_RGB;
		}
		else if (channels == 4)
		{
			internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
			dataFormat = GL_RGBA;
		}
		else
		{
			return false;
		}

		return true;
	}

	inline bool IsDepthFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::DEPTH24STENCIL8: return true;
		case TextureFormat::DEPTH32F: return true;
		}

		return false;
	}

	inline GLenum GetDepthAttachmentType(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::DEPTH24STENCIL8: return GL_DEPTH_STENCIL_ATTACHMENT;
		case TextureFormat::DEPTH32F: return GL_DEPTH_ATTACHMENT;
		}

		ATN_CORE_ASSERT(false);
		return GL_NONE;
	}

	inline GLenum TextureCubeTargetToGLenum(TextureCubeTarget target)
	{
		switch (target)
		{
		case TextureCubeTarget::POSITIVE_X: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		case TextureCubeTarget::NEGATIVE_X: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
		case TextureCubeTarget::POSITIVE_Y: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
		case TextureCubeTarget::NEGATIVE_Y: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
		case TextureCubeTarget::POSITIVE_Z: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
		case TextureCubeTarget::NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
		}

		ATN_CORE_ASSERT(false, "Unknown texture target!");
		return GL_NONE;
	}

	inline GLenum ShaderDataTypeToGLBaseType(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Float:  return GL_FLOAT;
		case ShaderDataType::Float2: return GL_FLOAT;
		case ShaderDataType::Float3: return GL_FLOAT;
		case ShaderDataType::Float4: return GL_FLOAT;
		case ShaderDataType::Mat3:	 return GL_FLOAT;
		case ShaderDataType::Mat4:   return GL_FLOAT;
		case ShaderDataType::Int:    return GL_INT;
		case ShaderDataType::Int2:   return GL_INT;
		case ShaderDataType::Int3:   return GL_INT;
		case ShaderDataType::Int4:   return GL_INT;
		case ShaderDataType::Bool:   return GL_BOOL;
		}

		ATN_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

	inline GLenum CullFaceToGLenum(CullFace face)
	{
		switch (face)
		{
		case CullFace::BACK: return GL_BACK;
		case CullFace::FRONT: return GL_FRONT;
		}

		ATN_CORE_ASSERT(false);
		return GL_NONE;
	}

	inline GLenum CullDirectionToGLenum(CullDirection direction)
	{
		switch (direction)
		{
		case CullDirection::CLOCKWISE: return GL_CW;
		case CullDirection::COUNTER_CLOCKWISE: return GL_CCW;
		}

		ATN_CORE_ASSERT(false);
		return GL_NONE;
	}

	inline GLenum ShaderTypeToGLenum(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::VERTEX_SHADER: return GL_VERTEX_SHADER;
		case ShaderType::FRAGMENT_SHADER: return GL_FRAGMENT_SHADER;
		case ShaderType::GEOMETRY_SHADER: return GL_GEOMETRY_SHADER;
		case ShaderType::COMPUTE_SHADER: return GL_COMPUTE_SHADER;
		}

		ATN_CORE_ASSERT(false, "Unknown shader type!");
		return 0;
	}
}
