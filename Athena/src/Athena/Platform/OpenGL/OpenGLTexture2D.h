#pragma once

#include "Athena/Renderer/Texture.h"


namespace Athena
{
	class ATHENA_API OpenGLTexture2D: public Texture2D
	{
	public:
		OpenGLTexture2D(const std::string& path);
		~OpenGLTexture2D();

		inline uint32_t GetWidth() const override { return m_Width; }
		inline uint32_t GetHeight() const override { return m_Height; }

		void Bind(uint32_t slot = 0) const override;

	private:
		uint32_t m_RendererID;

		std::string m_Path;
		uint32_t m_Width, m_Height;
	};
}

