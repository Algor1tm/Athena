#include "GLTextureSampler.h"

#include "Athena/Platform/OpenGL/GLUtils.h"

#include <glad/glad.h>


namespace Athena
{
	GLTextureSampler::GLTextureSampler(const TextureSamplerCreateInfo& info)
	{
		glGenSamplers(1, &m_RendererID);

		glSamplerParameteri(m_RendererID, GL_TEXTURE_WRAP_S, Utils::TextureWrapToGLenum(info.Wrap));
		glSamplerParameteri(m_RendererID, GL_TEXTURE_WRAP_T, Utils::TextureWrapToGLenum(info.Wrap));
		glSamplerParameteri(m_RendererID, GL_TEXTURE_WRAP_R, Utils::TextureWrapToGLenum(info.Wrap));
		glSamplerParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, Utils::TextureFilterToGLenum(info.MinFilter));
		glSamplerParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, Utils::TextureFilterToGLenum(info.MagFilter));

		glSamplerParameterfv(m_RendererID, GL_TEXTURE_BORDER_COLOR, info.BorderColor.Data());

		if(info.CompareMode != TextureCompareMode::NONE)
			glSamplerParameteri(m_RendererID, GL_TEXTURE_COMPARE_MODE, Utils::TextureCompareModeToGLenum(info.CompareMode));

		if (info.CompareFunc != TextureCompareFunc::NONE)
			glSamplerParameteri(m_RendererID, GL_TEXTURE_COMPARE_FUNC, Utils::TextureCompareFuncToGLenum(info.CompareFunc));
	}

	GLTextureSampler::~GLTextureSampler()
	{
		glDeleteSamplers(1, &m_RendererID);
	}

	void GLTextureSampler::Bind(uint32 slot) const
	{
		glBindSampler(slot, m_RendererID);
	}

	void GLTextureSampler::UnBind(uint32 slot) const
	{
		glBindSampler(slot, 0);
	}
}
