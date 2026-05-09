#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Asset/Asset.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	class FontGeometry;

	class ATHENA_API Font: public Asset
	{
	public:
		Font();
		~Font();

		virtual AssetType GetAssetType() const override { return AssetType::Font; }

		virtual bool Serialize(const FilePath& absolutePath) const override;
		virtual bool Deserialize(const FilePath& absolutePath) override;

		Ref<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }
		FontGeometry* GetFontGeometry() { return m_FontGeometry; }

		friend class FontImporter;

	public:
		static bool Init();
		static void Shutdown();

		static Ref<Font> GetDefault();
		static void* GetFTPHandle();

	private:
		struct FontStaticData
		{
			void* FTPHandle;
			Ref<Font> DefaultFont;
		};

		static FontStaticData s_Data;

		FontGeometry* m_FontGeometry;
		Ref<Texture2D> m_AtlasTexture;
	};
}
