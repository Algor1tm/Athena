#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Asset/Asset.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	class FontGeometry;

	class ATHENA_API Font : public Asset
	{
	public:
		static bool Init();
		static void Shutdown();

		static Ref<Font> Create(const FilePath& path);
		~Font();

		static Ref<Font> GetDefault();

		virtual AssetType GetType() const override { return AssetType::Font; }

		Ref<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }
		FontGeometry* GetFontGeometry() { return m_FontGeometry; }

	private:
		Buffer GenerateAtlasOrReadFromCache(const FilePath& path, uint32 width, uint32 height);

	private:
		FontGeometry* m_FontGeometry = nullptr;
		Ref<Texture2D> m_AtlasTexture;
	};
}
