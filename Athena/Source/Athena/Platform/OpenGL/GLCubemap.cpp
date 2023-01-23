#include "GLCubemap.h"

#include <glad/glad.h>
#include <stb_image/stb_image.h>


namespace Athena
{
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

			ATN_CORE_ASSERT(internalFormat * dataFormat, "Texture format not supported!");

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

			stbi_image_free(data);
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
