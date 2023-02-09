#include "GLCubemap.h"

#include "Athena/Platform/OpenGL/Shared.h"

#include <glad/glad.h>
#include <stb_image/stb_image.h>


namespace Athena
{
	GLCubemap::GLCubemap(const CubemapDescription& desc)
	{
		if (!desc.Faces[0].second.empty())
		{
			LoadFromFile(desc);
		}
		else
		{
			PreAllocate(desc);
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

	void GLCubemap::LoadFromFile(const CubemapDescription& desc)
	{
		stbi_set_flip_vertically_on_load(false);

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_GLRendererID);
		m_RendererID = m_GLRendererID;

		ApplyTexParameters(desc);

		for (const auto& [target, path] : desc.Faces)
		{
			int width, height, channels;

			unsigned char* data;
			data = stbi_load(path.string().data(), &width, &height, &channels, 0);

			if (!data)
			{
				ATN_CORE_ERROR("Failed to load texture for Cubemap! (path = '{0}')", path);
				m_IsLoaded = false;
				break;
			}

			GLenum internalFormat = 0, dataFormat = 0;
			if (!GetGLFormats(channels, false, desc.sRGB, internalFormat, dataFormat))
			{
				ATN_CORE_ERROR("Texture format not supported(texture = {0}, channels = {1})", path, channels);
				m_IsLoaded = false;
				stbi_image_free(data);
				break;
			}

			glTexImage2D(GLTextureTarget(target), 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

			stbi_image_free(data);
		}
	}

	void GLCubemap::PreAllocate(const CubemapDescription& desc)
	{
		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_GLRendererID);
		m_RendererID = m_GLRendererID;

		ApplyTexParameters(desc);

		GLenum internalFormat, dataFormat, type;
		AthenaFormatToGLenum(desc.Format, internalFormat, dataFormat, type);
		
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_GLRendererID);

		for (uint32 i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, desc.Width, desc.Height, 0, dataFormat, type, nullptr);
		}

		m_IsLoaded = true;
	}

	void GLCubemap::ApplyTexParameters(const CubemapDescription& desc)
	{
		GLenum minfilter = AthenaTextureFilterToGLenum(desc.MinFilter);
		GLenum magfilter = AthenaTextureFilterToGLenum(desc.MagFilter);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MIN_FILTER, minfilter);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAG_FILTER, magfilter);

		GLenum wrap = AthenaTextureWrapToGLenum(desc.Wrap);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_S, wrap);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_T, wrap);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_R, wrap);
	}

	void GLCubemap::GenerateMipMap(uint32 count)
	{
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_BASE_LEVEL, 0);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAX_LEVEL, count);
		
		glGenerateTextureMipmap(m_GLRendererID);
	}
}
