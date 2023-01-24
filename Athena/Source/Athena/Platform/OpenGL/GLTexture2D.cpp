#include "GLTexture2D.h"
#include "Shared.h"

#include <glad/glad.h>
#include <stb_image/stb_image.h>


namespace Athena
{
	GLTexture2D::GLTexture2D(uint32 width, uint32 height)
		: m_Width(width), m_Height(height)
	{
		m_InternalFormat = GL_RGBA8;
		m_DataFormat = GL_RGBA;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_GLRendererID);
		m_RendererID = m_GLRendererID;
		glTextureStorage2D(m_GLRendererID, 1, m_InternalFormat, m_Width, m_Height);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	GLTexture2D::GLTexture2D(const Texture2DDescription& desc)
		: m_Path(desc.TexturePath)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);

		void* data;
		if(desc.HDR)
			data = stbi_loadf(m_Path.string().data(), &width, &height, &channels, 0); // float*
		else
			data = stbi_load(m_Path.string().data(), &width, &height, &channels, 0);  // unsigned char*

		if (data)
		{
			m_Width = width;
			m_Height = height;

			if (!GetGLFormats(channels, desc.HDR, m_InternalFormat, m_DataFormat))
			{
				ATN_CORE_ERROR("Texture format not supported(texture = {0}, channels = {1})", desc.TexturePath, channels);
				stbi_image_free(data);
				return;
			}

			m_IsLoaded = true;

			glCreateTextures(GL_TEXTURE_2D, 1, &m_GLRendererID);
			m_RendererID = m_GLRendererID;
			glTextureStorage2D(m_GLRendererID, 1, m_InternalFormat, m_Width, m_Height);

			glTextureParameteri(m_GLRendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTextureSubImage2D(m_GLRendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, desc.HDR ? GL_FLOAT : GL_UNSIGNED_BYTE, data);

			if (channels == 1)
			{
				const GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
				glTextureParameteriv(m_GLRendererID, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			}

			stbi_image_free(data);
		}
		else
		{
			ATN_CORE_ERROR("Failed to load image for Texture2D! (path = '{0}')", m_Path);
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
}
