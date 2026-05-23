#pragma once

#include "Athena/Core/Core.h"
#include "Panels/Panel.h"
#include "Athena/Asset/Editor/MeshImporter.h"


namespace Athena
{
	class MeshImportSettingsPanel : public Panel
	{
	public:
		MeshImportSettingsPanel(const Ref<EditorContext>& context);

		virtual void OnImGuiRender() override;
		void OnOpen(AssetHandle meshSourceHandle);

	private:
		void OnSave();
		void OnClose();

	private:
		AssetHandle m_MeshSourceHandle = 0;
		Ref<MeshImportSettings> m_ImportSettingsCopy;
	};
}
