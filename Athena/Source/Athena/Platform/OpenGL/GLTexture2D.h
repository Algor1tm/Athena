#pragma once

#include "Athena/Renderer/Texture.h"


typedef unsigned int GLenum;

namespace Athena
{
	class ATHENA_API GLTexture2D: public Texture2D
	{
	public:
		GLTexture2D(uint32 width, uint32 height);
		GLTexture2D(const Filepath& path);
		virtual ~GLTexture2D();

		virtual inline uint32 GetWidth() const override { return m_Width; }
		virtual inline uint32 GetHeight() const override { return m_Height; }

		virtual void SetData(const void* data, uint32 size) override;

		virtual void Bind(uint32 slot = 0) const override;
		virtual bool IsLoaded() const override { return m_IsLoaded; };

		virtual const Filepath& GetFilepath() const override { return m_Path; };

	private:
		Filepath m_Path;
		uint32 m_Width = 0, m_Height = 0;
		bool m_IsLoaded = false;
		GLenum m_InternalFormat = 0, m_DataFormat = 0;
		uint32 m_GLRendererID;
	};
}
