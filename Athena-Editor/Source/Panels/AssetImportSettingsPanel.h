#pragma once

#include "Athena/Core/Core.h"
#include "Panels/Panel.h"
#include "Athena/Asset/Editor/MeshImporter.h"
#include "Athena/Asset/Editor/TextureImporter.h"


namespace Athena
{
	class AssetImportSettingsPanel : public Panel
	{
	public:
		AssetImportSettingsPanel(const Ref<EditorContext>& context);

		virtual void OnImGuiRender() override;
		void OnOpen(AssetHandle assetHandle);

	private:
		void OnSave();
		void OnClose();
		void OnReset();

		void DrawMeshImportSettings();
		void DrawTextureImportSettings();
		void DrawEnvMapImportSettings();

	private:
		AssetHandle m_AssetHandle = 0;
		Ref<AssetImportSettings> m_ImportSettingsCopy;
	};
}
