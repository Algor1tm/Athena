#include "GLCubemap.h"
#include "Shared.h"

#include <glad/glad.h>
#include <stb_image/stb_image.h>


namespace Athena
{
	static void GetGLFormatAndType(TextureFormat format, GLenum& internalFormat, GLenum& dataFormat, GLenum& type)
	{
		switch (format)
		{
		case TextureFormat::RGB16F:
		{
			internalFormat = GL_RGB16F;
			dataFormat = GL_RGB;
			type = GL_FLOAT;
		}
		}

		ATN_CORE_ASSERT(false, "Invalid texture format!");
	}

	GLCubemap::GLCubemap(const std::array<Filepath, 6>& faces)
	{
		stbi_set_flip_vertically_on_load(false);

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_GLRendererID);
		m_RendererID = m_GLRendererID;

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		glBindTexture(GL_TEXTURE_CUBE_MAP, m_GLRendererID);
		m_IsLoaded = true;
		for (uint32 i = 0; i < faces.size(); ++i)
		{
			int width, height, channels;

			unsigned char* data;
			data = stbi_load(faces[i].string().data(), &width, &height, &channels, 0);

			if (!data)
			{
				ATN_CORE_ERROR("Failed to load texture for Cubemap! (path = '{0}')", faces[i]);
				m_IsLoaded = false;
				break;
			}

			GLenum internalFormat = 0, dataFormat = 0;
			if (!GetGLFormats(channels, false, internalFormat, dataFormat))
			{
				ATN_CORE_ERROR("Texture format not supported(texture = {0}, channels = {1})", faces[i], channels);
				m_IsLoaded = false;
				stbi_image_free(data);
				break;
			}

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

			stbi_image_free(data);
		}
	}

	GLCubemap::GLCubemap(uint32 width, uint32 height, TextureFormat format)
	{
		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_GLRendererID);
		m_RendererID = m_GLRendererID;

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		glBindTexture(GL_TEXTURE_CUBE_MAP, m_GLRendererID);
		m_IsLoaded = true;

		for (uint32 i = 0; i < 6; ++i)
		{
			GLenum internalFormat, dataFormat, type;
			GetGLFormatAndType(format, internalFormat, dataFormat, type);

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, dataFormat, type, nullptr);
		}
	}

	GLCubemap::~GLCubemap()
	{
		glDeleteTextures(1, &m_GLRendererID);
	}

	void GLCubemap::Bind(uint32 slot) const
	{
		glBindTextureUnit(slot, m_GLRendererID);
	}

	bool GLCubemap::IsLoaded() const
	{
		return m_IsLoaded;
	}
}
