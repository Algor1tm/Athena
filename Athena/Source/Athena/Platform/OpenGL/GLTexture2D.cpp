#include "GLTexture2D.h"

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

	GLTexture2D::GLTexture2D(const Filepath& path)
		: m_Path(path)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);

		unsigned char* data;
		data = stbi_load(path.string().data(), &width, &height, &channels, 0);

		if (data)
		{
			m_Width = width;
			m_Height = height;
			m_IsLoaded = true;

			GLenum internalFormat = 0, dataFormat = 0;
			if (channels == 4)
			{
				internalFormat = GL_RGBA8;
				dataFormat = GL_RGBA;
			}
			else if (channels == 3)
			{
				internalFormat = GL_RGB8;
				dataFormat = GL_RGB;
			}
			else if (channels == 1)
			{
				internalFormat = GL_R8;
				dataFormat = GL_RED;
			}
			
			m_InternalFormat = internalFormat;
			m_DataFormat = dataFormat;

			ATN_CORE_ASSERT(m_InternalFormat * m_DataFormat, "Texture format not supported!");

			glCreateTextures(GL_TEXTURE_2D, 1, &m_GLRendererID);
			m_RendererID = m_GLRendererID;
			glTextureStorage2D(m_GLRendererID, 1, internalFormat, m_Width, m_Height);

			glTextureParameteri(m_GLRendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTextureSubImage2D(m_GLRendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);

			if (channels == 1)
			{
				GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
				glTextureParameteriv(m_GLRendererID, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			}

			stbi_image_free(data);
		}
		else
		{
			ATN_CORE_ERROR("Failed to load image for Texture2D! (path = '{0}')", path);
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
