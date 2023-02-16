#include "GLCubemap.h"

#include "Athena/Platform/OpenGL/Shared.h"

#include <glad/glad.h>
#include <stb_image/stb_image.h>


namespace Athena
{
	static inline GLenum GLCubemapTarget(CubemapTarget target)
	{
		switch (target)
		{
		case CubemapTarget::TEXTURE_CUBE_MAP_POSITIVE_X: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		case CubemapTarget::TEXTURE_CUBE_MAP_NEGATIVE_X: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
		case CubemapTarget::TEXTURE_CUBE_MAP_POSITIVE_Y: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
		case CubemapTarget::TEXTURE_CUBE_MAP_NEGATIVE_Y: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
		case CubemapTarget::TEXTURE_CUBE_MAP_POSITIVE_Z: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
		case CubemapTarget::TEXTURE_CUBE_MAP_NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
		}

		ATN_CORE_ASSERT(false, "Unknown texture target!");
		return GL_NONE;
	}


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

			m_InternalFormat = internalFormat;
			m_DataFormat = dataFormat;

			glTexImage2D(GLCubemapTarget(target), 0, m_InternalFormat, width, height, 0, m_DataFormat, GL_UNSIGNED_BYTE, data);
			
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

		m_InternalFormat = internalFormat;
		m_DataFormat = dataFormat;

		glBindTexture(GL_TEXTURE_CUBE_MAP, m_GLRendererID);

		for (uint32 i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_InternalFormat, desc.Width, desc.Height, 0, m_DataFormat, type, nullptr);
		}

		m_IsLoaded = true;
	}

	void GLCubemap::ApplyTexParameters(const CubemapDescription& desc)
	{
		SetFilters(desc.MinFilter, desc.MagFilter);

		GLenum wrap = AthenaTextureWrapToGLenum(desc.Wrap);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_S, wrap);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_T, wrap);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_R, wrap);
	}

	void GLCubemap::GenerateMipMap(uint32 maxLevel)
	{
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_BASE_LEVEL, 0);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAX_LEVEL, maxLevel);
		
		glGenerateTextureMipmap(m_GLRendererID);
	}

	void GLCubemap::BindAsImage(uint32 slot, uint32 level)
	{
		glBindImageTexture(slot, m_RendererID, level, GL_TRUE, 0, GL_WRITE_ONLY, m_InternalFormat);
	}

	void GLCubemap::SetFilters(TextureFilter min, TextureFilter mag)
	{
		GLenum minfilter = AthenaTextureFilterToGLenum(min);
		GLenum magfilter = AthenaTextureFilterToGLenum(mag);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MIN_FILTER, minfilter);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAG_FILTER, magfilter);
	}
}
