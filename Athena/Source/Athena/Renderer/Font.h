#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	class ATHENA_API Font: public RefCounted
	{
	public:
		static bool Init();
		static void Shutdown();

		static Ref<Font> Create(const FilePath& path);

		static Ref<Font> GetDefault();

		Ref<Texture2D> GetTextureAtlas() const { return m_TextureAtlas; }
		const FilePath& GetFilePath() const { return m_FilePath; }

	private:
		FilePath m_FilePath;
		Ref<Texture2D> m_TextureAtlas;
	};
}
