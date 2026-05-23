#include "AssetImportSettingsPanel.h"
#include "Athena/Asset/AssetManager.h"
#include "Athena/Asset/Editor/AssetFileExtensions.h"
#include "Athena/Asset/Editor/MeshImporter.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Scene/Components.h"
#include "Athena/UI/UI.h"
#include "Athena/UI/Theme.h"

#include "Panels/PanelManager.h"

#include <ImGui/imgui.h>
#include <queue>


namespace Athena
{
    MeshImportSettingsPanel::MeshImportSettingsPanel(const Ref<EditorContext>& context)
		: Panel(MESH_IMPORT_SETTINGS_PANEL_ID, context)
	{

	}

	void MeshImportSettingsPanel::OnImGuiRender()
	{
        Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(m_MeshSourceHandle);

        if (!mesh || m_EditorCtx.SceneState != SceneState::Edit)
        {
            OnClose();
            return;
        }

        ImGui::Begin("Mesh Import Settings");

        if (UI::BeginPropertyTable())
        {
            UI::PropertyCheckbox("Import Animations", &m_ImportSettingsCopy->ImportAnimations);

            if (m_ImportSettingsCopy->ImportAnimations)
            {
                m_ImportSettingsCopy->CollapseGraph = true;

                ImGui::BeginDisabled();
                UI::PropertyCheckbox("Collapse Graph", &m_ImportSettingsCopy->CollapseGraph);
                ImGui::EndDisabled();
            }
            else
            {
                UI::PropertyCheckbox("Collapse Graph", &m_ImportSettingsCopy->CollapseGraph);
            }

            ImGui::BeginDisabled();
            UI::PropertyText("FilePath", AssetManager::GetAssetFilePath(m_MeshSourceHandle).string());
            ImGui::EndDisabled();

            UI::EndPropertyTable();
        }

        if (ImGui::Button("Save"))
        {
            OnSave();
            OnClose();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            OnClose();
        }

        ImGui::End();
	}

    void MeshImportSettingsPanel::OnOpen(AssetHandle meshSourceHandle)
    {
        if (!AssetManager::IsAssetHandleValid(meshSourceHandle))
        {
            PanelManager::ClosePanel(MESH_IMPORT_SETTINGS_PANEL_ID);
            return;
        }

        m_MeshSourceHandle = meshSourceHandle;

        Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(meshSourceHandle);
        m_ImportSettingsCopy = Ref<MeshImportSettings>::Create();

        Ref<MeshImportSettings> importSettings = Project::GetEditorAssetManager()->GetAssetImportSettings(meshSourceHandle).As<MeshImportSettings>();

        m_ImportSettingsCopy->ImportAnimations = importSettings->ImportAnimations;
        m_ImportSettingsCopy->CollapseGraph = importSettings->CollapseGraph;
        m_ImportSettingsCopy->SubMeshIndices = importSettings->SubMeshIndices;
        m_ImportSettingsCopy->OverrideMaterials = importSettings->OverrideMaterials;
    }

    void MeshImportSettingsPanel::OnSave()
    {
        Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(m_MeshSourceHandle);

        if (mesh)
        {
            Project::GetEditorAssetManager()->SetAssetImportSettings(m_MeshSourceHandle, m_ImportSettingsCopy);
            Project::GetEditorAssetManager()->SerializeAssetImportSettings(m_MeshSourceHandle); // Asset watcher thread will reload this asset
        }
    }

    void MeshImportSettingsPanel::OnClose()
    {
        m_MeshSourceHandle = 0;
        m_ImportSettingsCopy.Release();

        PanelManager::ClosePanel(MESH_IMPORT_SETTINGS_PANEL_ID);
    }
}
