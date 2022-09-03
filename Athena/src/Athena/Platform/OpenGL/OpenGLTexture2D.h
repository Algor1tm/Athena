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

		inline uint32 GetWidth() const override { return m_Width; }
		inline uint32 GetHeight() const override { return m_Height; }

		void SetData(const void* data, uint32 size) override;

		void Bind(uint32 slot = 0) const override;
		void UnBind() const override;

		const String& GetFilepath() const override { return m_Path; };

	private:
		String m_Path;
		uint32 m_Width, m_Height;
		GLenum m_InternalFormat, m_DataFormat;
	};
}
