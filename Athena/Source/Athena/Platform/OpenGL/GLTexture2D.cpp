#include "GLTexture2D.h"

#include "Athena/Platform/OpenGL/Shared.h"

#include <glad/glad.h>
#include <stb_image/stb_image.h>


namespace Athena
{
	GLTexture2D::GLTexture2D(const FilePath& filepath, bool sRGB, const TextureSamplerDescription& samplerDesc)
		: m_FilePath(filepath)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);

		bool HDR = false;
		String path = m_FilePath.string();
		void* data;
		if (stbi_is_hdr(path.data()))
		{
			data = stbi_loadf(path.data(), &width, &height, &channels, 0);
			HDR = true;
		}
		else
		{
			data = stbi_load(path.data(), &width, &height, &channels, 0);
		}

		if (data)
		{
			m_Width = width;
			m_Height = height;

			if (!GetGLFormats(channels, HDR, sRGB, m_InternalFormat, m_DataFormat))
			{
				ATN_CORE_ERROR("Texture format not supported(texture = {0}, channels = {1})", filepath, channels);
				stbi_image_free(data);
				return;
			}

			glCreateTextures(GL_TEXTURE_2D, 1, &m_GLRendererID);
			m_RendererID = m_GLRendererID;
			glTextureStorage2D(m_GLRendererID, 1, m_InternalFormat, m_Width, m_Height);

			CreateSampler(samplerDesc);

			glTextureSubImage2D(m_GLRendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, HDR ? GL_FLOAT : GL_UNSIGNED_BYTE, data);

			if (channels == 1)
			{
				const GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
				glTextureParameteriv(m_GLRendererID, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			}

			stbi_image_free(data);

			m_IsLoaded = true;
		}
		else
		{
			ATN_CORE_ERROR("Failed to load Texture2D!(path = '{0}')", m_FilePath);
		}
	}

	GLTexture2D::GLTexture2D(const void* Data, uint32 Width, uint32 Height, bool sRGB, const TextureSamplerDescription& samplerDesc)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);

		void* data;
		const uint32 size = Height == 0 ? Width : Width * Height;
		data = stbi_load_from_memory((const stbi_uc*)Data, size, &width, &height, &channels, 0);

		if (data)
		{
			m_Width = width;
			m_Height = height;

			if (!GetGLFormats(channels, false, sRGB, m_InternalFormat, m_DataFormat))
			{
				ATN_CORE_ERROR("Texture format not supported(channels = {})", channels);
				stbi_image_free(data);
				return;
			}

			glCreateTextures(GL_TEXTURE_2D, 1, &m_GLRendererID);
			m_RendererID = m_GLRendererID;
			glTextureStorage2D(m_GLRendererID, 1, m_InternalFormat, m_Width, m_Height);

			CreateSampler(samplerDesc);

			glTextureSubImage2D(m_GLRendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);

			if (channels == 1)
			{
				const GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
				glTextureParameteriv(m_GLRendererID, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			}

			stbi_image_free(data);

			m_IsLoaded = true;
		}
		else
		{
			ATN_CORE_ERROR("Failed to load Texture2D from memory!");
		}
	}

	GLTexture2D::GLTexture2D(TextureFormat Format, uint32 Width, uint32 Height, const TextureSamplerDescription& samplerDesc)
	{
		m_Width = Width;
		m_Height = Height;

		GLenum type;
		AthenaFormatToGLenum(Format, m_InternalFormat, m_DataFormat, type);

		glCreateTextures(GL_TEXTURE_2D, 1, &m_GLRendererID);
		m_RendererID = m_GLRendererID;
		glTextureStorage2D(m_GLRendererID, 1, m_InternalFormat, m_Width, m_Height);

		CreateSampler(samplerDesc);

		m_IsLoaded = true;
	}

	GLTexture2D::~GLTexture2D()
	{
		glDeleteTextures(1, &m_GLRendererID);
	}

	void GLTexture2D::Bind(uint32 slot) const
	{
		glBindTextureUnit(slot, m_GLRendererID);
	}

	void GLTexture2D::BindAsImage(uint32 slot, uint32 level) 
	{
		glBindImageTexture(slot, m_RendererID, level, GL_FALSE, 0, GL_WRITE_ONLY, m_InternalFormat);
	}

	void GLTexture2D::SetData(const void* data, uint32 size)
	{
		uint32 bpp = 0;
		switch (m_DataFormat)
		{
		case GL_RGBA: bpp = 4; break;
		case GL_RGB: bpp = 3; break;
		case GL_RED: bpp = 1; break;
		}

		if (size == m_Width * m_Height * bpp || (bpp == 1 && size == m_Width))
		{
			glTextureSubImage2D(m_GLRendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			ATN_CORE_ERROR("GLTexture2D::SetData: Data must be entire texture!");
		}
	}

	void GLTexture2D::CreateSampler(const TextureSamplerDescription& desc)
	{
		GLenum minfilter = AthenaTextureFilterToGLenum(desc.MinFilter);
		GLenum magfilter = AthenaTextureFilterToGLenum(desc.MagFilter);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MIN_FILTER, minfilter);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAG_FILTER, magfilter);

		GLenum wrap = AthenaTextureWrapToGLenum(desc.Wrap);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_S, wrap);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_T, wrap);

		glTextureParameterfv(m_GLRendererID, GL_TEXTURE_BORDER_COLOR, desc.BorderColor.Data());

		if (desc.CompareMode != TextureCompareMode::NONE)
			glTextureParameteri(m_GLRendererID, GL_TEXTURE_COMPARE_MODE, AthenaTextureCompareModeToGLenum(desc.CompareMode));

		if (desc.CompareFunc != TextureCompareFunc::NONE)
			glTextureParameteri(m_GLRendererID, GL_TEXTURE_COMPARE_FUNC, AthenaTextureCompareFuncToGLenum(desc.CompareFunc));

		if(desc.MinFilter == TextureFilter::LINEAR_MIPMAP_LINEAR || desc.MagFilter == TextureFilter::LINEAR_MIPMAP_LINEAR)
			glGenerateTextureMipmap(m_GLRendererID);
	}
}
