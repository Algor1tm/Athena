#include "GLTextureCube.h"

#include "Athena/Platform/OpenGL/Shared.h"

#include <glad/glad.h>
#include <stb_image/stb_image.h>


namespace Athena
{
	static inline GLenum GLTextureCubeTarget(TextureCubeTarget target)
	{
		switch (target)
		{
		case TextureCubeTarget::POSITIVE_X: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		case TextureCubeTarget::NEGATIVE_X: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
		case TextureCubeTarget::POSITIVE_Y: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
		case TextureCubeTarget::NEGATIVE_Y: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
		case TextureCubeTarget::POSITIVE_Z: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
		case TextureCubeTarget::NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
		}

		ATN_CORE_ASSERT(false, "Unknown texture target!");
		return GL_NONE;
	}


	GLTextureCube::GLTextureCube(const std::array<std::pair<TextureCubeTarget, FilePath>, 6>& faces, bool sRGB, const TextureSamplerDescription& samplerDesc)
	{
		stbi_set_flip_vertically_on_load(false);

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_GLRendererID);
		m_RendererID = m_GLRendererID;

		CreateSampler(samplerDesc);

		for (const auto& [target, path] : faces)
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
			if (!GetGLFormats(channels, false, sRGB, internalFormat, dataFormat))
			{
				ATN_CORE_ERROR("Texture format not supported(texture = {0}, channels = {1})", path, channels);
				m_IsLoaded = false;
				stbi_image_free(data);
				break;
			}

			m_InternalFormat = internalFormat;
			m_DataFormat = dataFormat;

			glTexImage2D(GLTextureCubeTarget(target), 0, m_InternalFormat, width, height, 0, m_DataFormat, GL_UNSIGNED_BYTE, data);

			stbi_image_free(data);
		}

		m_IsLoaded = true;
	}

	GLTextureCube::GLTextureCube(TextureFormat format, uint32 width, uint32 height, const TextureSamplerDescription& samplerDesc)
	{
		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_GLRendererID);
		m_RendererID = m_GLRendererID;

		CreateSampler(samplerDesc);

		GLenum internalFormat, dataFormat, type;
		AthenaFormatToGLenum(format, internalFormat, dataFormat, type);

		m_InternalFormat = internalFormat;
		m_DataFormat = dataFormat;

		glBindTexture(GL_TEXTURE_CUBE_MAP, m_GLRendererID);

		for (uint32 i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_InternalFormat, width, height, 0, m_DataFormat, type, nullptr);
		}

		m_IsLoaded = true;
	}

	GLTextureCube::~GLTextureCube()
	{
		glDeleteTextures(1, &m_GLRendererID);
	}

	void GLTextureCube::Bind(uint32 slot) const
	{
		glBindTextureUnit(slot, m_GLRendererID);
	}

	bool GLTextureCube::IsLoaded() const
	{
		return m_IsLoaded;
	}

	void GLTextureCube::CreateSampler(const TextureSamplerDescription& desc)
	{
		GLenum minfilter = AthenaTextureFilterToGLenum(desc.MinFilter);
		GLenum magfilter = AthenaTextureFilterToGLenum(desc.MagFilter);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MIN_FILTER, minfilter);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAG_FILTER, magfilter);

		GLenum wrap = AthenaTextureWrapToGLenum(desc.Wrap);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_S, wrap);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_T, wrap);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_WRAP_R, wrap);

		glTextureParameterfv(m_GLRendererID, GL_TEXTURE_BORDER_COLOR, desc.BorderColor.Data());

		if (desc.CompareMode != TextureCompareMode::NONE)
			glTextureParameteri(m_GLRendererID, GL_TEXTURE_COMPARE_MODE, AthenaTextureCompareModeToGLenum(desc.CompareMode));

		if (desc.CompareFunc != TextureCompareFunc::NONE)
			glTextureParameteri(m_GLRendererID, GL_TEXTURE_COMPARE_FUNC, AthenaTextureCompareFuncToGLenum(desc.CompareFunc));

		if (desc.MinFilter == TextureFilter::LINEAR_MIPMAP_LINEAR || desc.MagFilter == TextureFilter::LINEAR_MIPMAP_LINEAR)
			glGenerateTextureMipmap(m_GLRendererID);
	}

	void GLTextureCube::GenerateMipMap(uint32 maxLevel)
	{
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_BASE_LEVEL, 0);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAX_LEVEL, maxLevel);
		
		glGenerateTextureMipmap(m_GLRendererID);
	}

	void GLTextureCube::BindAsImage(uint32 slot, uint32 level)
	{
		glBindImageTexture(slot, m_RendererID, level, GL_TRUE, 0, GL_WRITE_ONLY, m_InternalFormat);
	}

	void GLTextureCube::SetFilters(TextureFilter min, TextureFilter mag)
	{
		GLenum minfilter = AthenaTextureFilterToGLenum(min);
		GLenum magfilter = AthenaTextureFilterToGLenum(mag);

		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MIN_FILTER, minfilter);
		glTextureParameteri(m_GLRendererID, GL_TEXTURE_MAG_FILTER, magfilter);
	}
}
