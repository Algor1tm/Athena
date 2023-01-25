#include "GLCubemap.h"
#include "Shared.h"

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

		ApplyTexParamters(desc);

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
			if (!GetGLFormats(channels, desc.HDR, desc.sRGB, internalFormat, dataFormat))
			{
				ATN_CORE_ERROR("Texture format not supported(texture = {0}, channels = {1})", path, channels);
				m_IsLoaded = false;
				stbi_image_free(data);
				break;
			}

			glTexImage2D(GLTextureTarget(target), 0, internalFormat, width, height, 0, dataFormat, desc.HDR ? GL_FLOAT : GL_UNSIGNED_BYTE, data);

			stbi_image_free(data);
		}
	}

	void GLCubemap::PreAllocate(const CubemapDescription& desc)
	{
		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_GLRendererID);
		m_RendererID = m_GLRendererID;

		ApplyTexParamters(desc);

		glBindTexture(GL_TEXTURE_CUBE_MAP, m_GLRendererID);

		GLenum internalFormat, dataFormat, type;
		GetGLFormatAndType(desc.Format, internalFormat, dataFormat, type);

		for (uint32 i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, desc.Width, desc.Height, 0, dataFormat, type, nullptr);
		}

		m_IsLoaded = true;
	}

	void GLCubemap::ApplyTexParamters(const CubemapDescription& desc)
	{
		GLenum minfilter = AthenaTextureFilterToGLenum(desc.MinFilter);
		GLenum magfilter = AthenaTextureFilterToGLenum(desc.MagFilter);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MIN_FILTER, minfilter);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAG_FILTER, magfilter);

		GLenum wrap = AthenaTextureWrapToGLenum(desc.Wrap);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_S, wrap);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_T, wrap);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_R, wrap);

		if (desc.MinFilter == TextureFilter::LINEAR_MIPMAP_LINEAR || desc.MagFilter == TextureFilter::LINEAR_MIPMAP_LINEAR)
			glGenerateTextureMipmap(m_GLRendererID);
	}
}
