#include "GLTextureSampler.h"

#include "Athena/Platform/OpenGL/GLUtils.h"

#include <glad/glad.h>


namespace Athena
{
	GLTextureSampler::GLTextureSampler(const TextureSamplerDescription& desc)
	{
		glGenSamplers(1, &m_RendererID);

		glSamplerParameteri(m_RendererID, GL_TEXTURE_WRAP_S, Utils::TextureWrapToGLenum(desc.Wrap));
		glSamplerParameteri(m_RendererID, GL_TEXTURE_WRAP_T, Utils::TextureWrapToGLenum(desc.Wrap));
		glSamplerParameteri(m_RendererID, GL_TEXTURE_WRAP_R, Utils::TextureWrapToGLenum(desc.Wrap));
		glSamplerParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, Utils::TextureFilterToGLenum(desc.MinFilter));
		glSamplerParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, Utils::TextureFilterToGLenum(desc.MagFilter));

		glSamplerParameterfv(m_RendererID, GL_TEXTURE_BORDER_COLOR, desc.BorderColor.Data());

		if(desc.CompareMode != TextureCompareMode::NONE)
			glSamplerParameteri(m_RendererID, GL_TEXTURE_COMPARE_MODE, Utils::TextureCompareModeToGLenum(desc.CompareMode));

		if (desc.CompareFunc != TextureCompareFunc::NONE)
			glSamplerParameteri(m_RendererID, GL_TEXTURE_COMPARE_FUNC, Utils::TextureCompareFuncToGLenum(desc.CompareFunc));
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
