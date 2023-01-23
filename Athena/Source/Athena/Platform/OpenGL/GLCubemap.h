#pragma once

#include "Athena/Renderer/Texture.h"


typedef unsigned int GLenum;

namespace Athena
{
	class ATHENA_API GLCubemap : public Cubemap
	{
	public:
		GLCubemap(const std::array<Filepath, 6>& faces);
		~GLCubemap();

		virtual void Bind(uint32 slot = 0) const override;
		virtual bool IsLoaded() const override;

	private:
		GLenum m_GLRendererID;
		bool m_IsLoaded = false;
	};
}
