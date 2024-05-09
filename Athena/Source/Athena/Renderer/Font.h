#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	class FontAtlasGeometryData;

	class ATHENA_API Font: public RefCounted
	{
	public:
		static bool Init();
		static void Shutdown();

		static Ref<Font> Create(const FilePath& path);
		~Font();

		static Ref<Font> GetDefault();

		Ref<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }
		const FilePath& GetFilePath() const { return m_FilePath; }
		const FontAtlasGeometryData* GetAtlasGeometryData() const { return m_AtlasGeometryData; }

	private:
		Buffer GenerateAtlasOrReadFromCache(uint32 width, uint32 height);

	private:
		FilePath m_FilePath;
		FontAtlasGeometryData* m_AtlasGeometryData;
		Ref<Texture2D> m_AtlasTexture;
	};
}
