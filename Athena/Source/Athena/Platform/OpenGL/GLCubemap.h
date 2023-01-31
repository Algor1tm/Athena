#pragma once

#include "Athena/Renderer/Texture.h"


typedef unsigned int GLenum;

namespace Athena
{
	class ATHENA_API GLCubemap : public Cubemap
	{
	public:
		GLCubemap(const CubemapDescription& desc);
		~GLCubemap();

		virtual void Bind(uint32 slot = 0) const override;
		virtual bool IsLoaded() const override;

		virtual void GenerateMipMap(uint32 count) override;

	private:
		void LoadFromFile(const CubemapDescription& desc);
		void PreAllocate(const CubemapDescription& desc);
		void ApplyTexParameters(const CubemapDescription& desc);

	private:
		GLenum m_GLRendererID;
		bool m_IsLoaded = false;
	};
}
