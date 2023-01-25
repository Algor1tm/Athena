#include "GLTexture2D.h"
#include "Shared.h"

#include <glad/glad.h>
#include <stb_image/stb_image.h>


namespace Athena
{
	GLTexture2D::GLTexture2D(const Texture2DDescription& desc)
		: m_Path(desc.TexturePath)
	{
		if (!desc.TexturePath.empty())
		{
			LoadFromFile(desc);
		}
		else if (desc.Data)
		{
			LoadFromMemory(desc);
		}
		else
		{
			PreAllocate(desc);
		}
	}

	GLTexture2D::~GLTexture2D()
	{
		glDeleteTextures(1, &m_GLRendererID);
	}

	void GLTexture2D::Bind(uint32 slot) const
	{
		glBindTextureUnit(slot, m_GLRendererID);
	}

	void GLTexture2D::SetData(const void* data, uint32 size)
	{
		uint32 bpp = m_DataFormat == GL_RGBA ? 4 : 3;
		ATN_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");

		glTextureSubImage2D(m_GLRendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
	}

	void GLTexture2D::LoadFromFile(const Texture2DDescription& desc)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);

		void* data;
		if (desc.HDR)
			data = stbi_loadf(m_Path.string().data(), &width, &height, &channels, 0); // float*
		else
			data = stbi_load(m_Path.string().data(), &width, &height, &channels, 0);  // unsigned char*

		if (data)
		{
			m_Width = width;
			m_Height = height;

			if (!GetGLFormats(channels, desc.HDR, desc.sRGB, m_InternalFormat, m_DataFormat))
			{
				ATN_CORE_ERROR("Texture format not supported(texture = {0}, channels = {1})", desc.TexturePath, channels);
				stbi_image_free(data);
				return;
			}


			glCreateTextures(GL_TEXTURE_2D, 1, &m_GLRendererID);
			m_RendererID = m_GLRendererID;
			glTextureStorage2D(m_GLRendererID, 1, m_InternalFormat, m_Width, m_Height);

			ApplyTexParamters(desc);

			glTextureSubImage2D(m_GLRendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, desc.HDR ? GL_FLOAT : GL_UNSIGNED_BYTE, data);

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
			ATN_CORE_ERROR("Failed to load Texture2D!(path = '{0}')", m_Path);
		}
	}

	void GLTexture2D::LoadFromMemory(const Texture2DDescription& desc)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);

		void* data;
		const uint32 size = desc.Height == 0 ? desc.Width : desc.Height * desc.Height;
		data = stbi_load_from_memory((const stbi_uc*)desc.Data, size, &width, &height, &channels, 0);

		if (data)
		{
			m_Width = width;
			m_Height = height;

			if (!GetGLFormats(channels, desc.HDR, desc.sRGB, m_InternalFormat, m_DataFormat))
			{
				ATN_CORE_ERROR("Texture format not supported(channels = {})", channels);
				stbi_image_free(data);
				return;
			}

			glCreateTextures(GL_TEXTURE_2D, 1, &m_GLRendererID);
			m_RendererID = m_GLRendererID;
			glTextureStorage2D(m_GLRendererID, 1, m_InternalFormat, m_Width, m_Height);

			ApplyTexParamters(desc);

			glTextureSubImage2D(m_GLRendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, desc.HDR ? GL_FLOAT : GL_UNSIGNED_BYTE, data);

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

	void GLTexture2D::PreAllocate(const Texture2DDescription& desc)
	{
		m_Width = desc.Width;
		m_Height = desc.Height;

		GLenum type;
		GetGLFormatAndType(desc.Format, m_InternalFormat, m_DataFormat, type);

		glCreateTextures(GL_TEXTURE_2D, 1, &m_GLRendererID);
		m_RendererID = m_GLRendererID;
		glTextureStorage2D(m_GLRendererID, 1, m_InternalFormat, m_Width, m_Height);

		ApplyTexParamters(desc);

		m_IsLoaded = true;
	}

	void GLTexture2D::ApplyTexParamters(const Texture2DDescription& desc)
	{
		GLenum minfilter = AthenaTextureFilterToGLenum(desc.MinFilter);
		GLenum magfilter = AthenaTextureFilterToGLenum(desc.MagFilter);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MIN_FILTER, minfilter);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAG_FILTER, magfilter);

		GLenum wrap = AthenaTextureWrapToGLenum(desc.Wrap);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_S, wrap);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_T, wrap);

		if(desc.MinFilter == TextureFilter::LINEAR_MIPMAP_LINEAR || desc.MagFilter == TextureFilter::LINEAR_MIPMAP_LINEAR)
			glGenerateTextureMipmap(m_GLRendererID);
	}
}
