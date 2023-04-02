#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Texture.h"


typedef unsigned int GLenum;

namespace Athena
{
	class ATHENA_API GLTextureSampler : public TextureSampler
	{
	public:
		GLTextureSampler(const TextureSamplerDescription& desc);
		~GLTextureSampler();

		virtual void Bind(uint32 slot) const override;
		virtual void UnBind(uint32 slot) const override;

	private:
		GLenum m_RendererID;
	};
}