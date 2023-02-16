#pragma once

#include "Athena/Core/Core.h"

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

		virtual void GenerateMipMap(uint32 maxLevel) override;
		virtual void BindAsImage(uint32 slot = 0, uint32 level = 0) override;

		virtual void SetFilters(TextureFilter min, TextureFilter mag) override;

	private:
		void LoadFromFile(const CubemapDescription& desc);
		void PreAllocate(const CubemapDescription& desc);
		void ApplyTexParameters(const CubemapDescription& desc);

	private:
		GLenum m_GLRendererID;
		bool m_IsLoaded = false;
		GLenum m_InternalFormat;
		GLenum m_DataFormat;
	};
}
