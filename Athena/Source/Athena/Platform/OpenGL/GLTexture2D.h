#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Texture.h"


typedef unsigned int GLenum;

namespace Athena
{
	class ATHENA_API GLTexture2D: public Texture2D
	{
	public:
		GLTexture2D(const Texture2DDescription& desc);
		virtual ~GLTexture2D();

		virtual inline uint32 GetWidth() const override { return m_Width; }
		virtual inline uint32 GetHeight() const override { return m_Height; }

		virtual void SetData(const void* data, uint32 size) override;

		virtual void Bind(uint32 slot = 0) const override;
		virtual bool IsLoaded() const override { return m_IsLoaded; };

		virtual const FilePath& GetFilePath() const override { return m_FilePath; };

	private:
		void LoadFromFile(const Texture2DDescription& desc);
		void LoadFromMemory(const Texture2DDescription& desc);
		void PreAllocate(const Texture2DDescription& desc);
		void ApplyTexParameters(const Texture2DDescription& desc);

	private:
		FilePath m_FilePath;
		uint32 m_Width = 0, m_Height = 0;
		bool m_IsLoaded = false;
		GLenum m_InternalFormat = 0, m_DataFormat = 0;
		uint32 m_GLRendererID;
	};
}
