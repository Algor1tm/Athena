#include "atnpch.h"
#include "OpenGLTexture2D.h"

#include <stb_image.h>


namespace Athena
{
	OpenGLTexture2D::OpenGLTexture2D(uint32 width, uint32 height)
		: m_Width(width), m_Height(height)
	{
		ATN_PROFILE_FUNCTION();

		m_InternalFormat = GL_RGBA8;
		m_DataFormat = GL_RGBA;

		glGenTextures(1, &m_RendererID);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_DataFormat, GL_UNSIGNED_BYTE, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	OpenGLTexture2D::OpenGLTexture2D(const String& path)
		: m_Path(path)
	{
		ATN_PROFILE_FUNCTION();

		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);

		unsigned char* data;
		{
			ATN_PROFILE_SCOPE("stbi_load - OpenGLTexture2D::OpenGLTexture2D(const String& path)");
			data = stbi_load(path.data(), &width, &height, &channels, 0);
		}

		ATN_CORE_ASSERT(data, "Failed to load image for Texture2D!");
		m_Width = width;
		m_Height = height;

		GLenum internalFormat = 0, dataFormat = 0;
		if (channels == 4)
		{
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		}
		else if(channels == 3)
		{
			internalFormat = GL_RGB8;
			dataFormat = GL_RGB;
		}

		m_InternalFormat = internalFormat;
		m_DataFormat = dataFormat;

		ATN_CORE_ASSERT(m_InternalFormat * m_DataFormat, "Texture format not supported!");

		glGenTextures(1, &m_RendererID);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_DataFormat, GL_UNSIGNED_BYTE, data);
		glBindTexture(GL_TEXTURE_2D, 0);

		stbi_image_free(data);
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		ATN_PROFILE_FUNCTION();

		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture2D::Bind(uint32 slot) const
	{
		ATN_PROFILE_FUNCTION();

		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
	}

	void OpenGLTexture2D::SetData(const void* data, uint32 size)
	{
		ATN_PROFILE_FUNCTION();

		uint32 bpp = m_DataFormat == GL_RGBA ? 4 : 3;
		ATN_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");

		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_DataFormat, GL_UNSIGNED_BYTE, data);
	}

	void OpenGLTexture2D::UnBind() const
	{
		ATN_PROFILE_FUNCTION();

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	bool OpenGLTexture2D::operator==(const Texture2D& other) const
	{
		return m_RendererID == ((OpenGLTexture2D&)other).m_RendererID;
	}
}
