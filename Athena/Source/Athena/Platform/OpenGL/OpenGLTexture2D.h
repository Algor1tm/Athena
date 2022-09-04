#pragma once

#include "Athena/Renderer/Texture.h"

#include <glad/glad.h>


namespace Athena
{
	class ATHENA_API OpenGLTexture2D: public Texture2D
	{
	public:
		OpenGLTexture2D(uint32 width, uint32 height);
		OpenGLTexture2D(const String& path);
		~OpenGLTexture2D();

		virtual inline uint32 GetWidth() const override { return m_Width; }
		virtual inline uint32 GetHeight() const override { return m_Height; }

		virtual void SetData(const void* data, uint32 size) override;

		virtual void Bind(uint32 slot = 0) const override;
		virtual bool IsLoaded() const override { return m_IsLoaded; };

		virtual const String& GetFilepath() const override { return m_Path; };

	private:
		String m_Path;
		uint32 m_Width = 0, m_Height = 0;
		bool m_IsLoaded = false;
		GLenum m_InternalFormat = 0, m_DataFormat = 0;
	};
}
