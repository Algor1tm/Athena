#include "AssetImportSettingsPanel.h"
#include "Athena/Asset/AssetManager.h"
#include "Athena/Asset/Editor/AssetFileExtensions.h"
#include "Athena/Asset/Editor/MeshImporter.h"
#include "Athena/Asset/Editor/TextureImporter.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Scene/Components.h"
#include "Athena/UI/UI.h"
#include "Athena/UI/Theme.h"

#include "Panels/PanelManager.h"

#include <ImGui/imgui.h>


namespace Athena
{
    AssetImportSettingsPanel::AssetImportSettingsPanel(const Ref<EditorContext>& context)
        : Panel(ASSET_IMPORT_SETTINGS_PANEL_ID, context)
    {
        UI::RegisterEnum(ATN_STRINGIFY_MACRO(TextureFilter));
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureFilter), 1, "Nearest");
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureFilter), 2, "Linear");
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureFilter), 3, "Trilinear");

        UI::RegisterEnum(ATN_STRINGIFY_MACRO(TextureWrap));
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureWrap), 1, "Repeat");
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureWrap), 2, "Clamp to edge");
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureWrap), 3, "Clamp to border");
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureWrap), 4, "Mirrored repeat");
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureWrap), 5, "Mirrored clamp to edge");
    }

    void AssetImportSettingsPanel::OnImGuiRender()
    {
        if (!AssetManager::IsAssetHandleValid(m_AssetHandle) || m_EditorCtx.SceneState != SceneState::Edit || !m_ImportSettingsCopy)
        {
            OnClose();
            return;
        }

        ImGui::Begin("Asset Import Settings");

        AssetType type = AssetManager::GetAssetType(m_AssetHandle);

        if (type == AssetType::Mesh)
        {
            DrawMeshImportSettings();
        }
        else if (type == AssetType::Texture)
        {
            DrawTextureImportSettings();
        }

        UI::PushFont(UI::Fonts::Bold);
        ImGui::Text(AssetManager::GetAssetFilePath(m_AssetHandle).string().data());
        UI::PopFont();

        if (ImGui::Button("Save"))
        {
            OnSave();
            OnClose();
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset"))
        {
            OnReset();
            OnClose();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            OnClose();
        }

        ImGui::End();
    }

    bool AssetImportSettingsPanel::SupportsAssetType(AssetType type)
    {
        if (type == AssetType::Mesh || type == AssetType::Texture)
            return true;

        return false;
    }

    void AssetImportSettingsPanel::OnOpen(AssetHandle assetHandle)
    {
        if (!AssetManager::IsAssetHandleValid(assetHandle) || !SupportsAssetType(AssetManager::GetAssetType(assetHandle)))
        {
            PanelManager::ClosePanel(ASSET_IMPORT_SETTINGS_PANEL_ID);
            return;
        }

        m_AssetHandle = assetHandle;

        Ref<AssetImportSettings> importSettings = Project::GetEditorAssetManager()->GetAssetImportSettings(m_AssetHandle);
        if(importSettings)
            m_ImportSettingsCopy = importSettings->Clone();
    }

    void AssetImportSettingsPanel::OnSave()
    {
        if (AssetManager::IsAssetHandleValid(m_AssetHandle))
        {
            Project::GetEditorAssetManager()->SetAssetImportSettings(m_AssetHandle, m_ImportSettingsCopy);
            Project::GetEditorAssetManager()->SerializeAssetImportSettings(m_AssetHandle); // Asset watcher thread will reload this asset
        }
    }

    void AssetImportSettingsPanel::OnClose()
    {
        m_AssetHandle = 0;
        m_ImportSettingsCopy.Release();

        PanelManager::ClosePanel(ASSET_IMPORT_SETTINGS_PANEL_ID);
    }

    void AssetImportSettingsPanel::OnReset()
    {
        if (AssetManager::IsAssetHandleValid(m_AssetHandle))
        {
            Ref<AssetImportSettings> importSettings = Project::GetEditorAssetManager()->GetDefaultImportSettings(AssetManager::GetAssetType(m_AssetHandle));
            m_ImportSettingsCopy = importSettings->Clone();
        }
    }

    void AssetImportSettingsPanel::DrawMeshImportSettings()
    {
        Ref<MeshImportSettings> settings = m_ImportSettingsCopy.As<MeshImportSettings>();

        UI::TextCentered("MESH IMPORT SETTINGS");

        if (UI::BeginPropertyTable())
        {
            UI::PropertyCheckbox("Import Animations", &settings->ImportAnimations);

            if (settings->ImportAnimations)
            {
                settings->CollapseGraph = true;

                ImGui::BeginDisabled();
                UI::PropertyCheckbox("Collapse Graph", &settings->CollapseGraph);
                ImGui::EndDisabled();
            }
            else
            {
                UI::PropertyCheckbox("Collapse Graph", &settings->CollapseGraph);
            }

            UI::EndPropertyTable();
        }
    }

    void AssetImportSettingsPanel::DrawTextureImportSettings()
    {
        Ref<TextureImportSettings> settings = m_ImportSettingsCopy.As<TextureImportSettings>();

        UI::TextCentered("TEXTURE IMPORT SETTINGS");

        if (UI::BeginPropertyTable())
        {
            UI::PropertyCheckbox("sRGB", &settings->sRGB);
            UI::PropertyCheckbox("GenerateMipMaps", &settings->GenerateMipMaps);
            UI::PropertyEnumCombo("WrapMode", ATN_STRINGIFY_MACRO(TextureWrap), (void*)&settings->WrapMode);
            UI::PropertyEnumCombo("FilterMode", ATN_STRINGIFY_MACRO(TextureFilter), (void*)&settings->FilterMode);
            UI::PropertySlider("Anisotropy Level", &settings->AnisotropyLevel, 0.f, Renderer::GetRenderCaps().MaxSamplerAnisotropy);
            UI::PropertyCheckbox("ComputeUsage", &settings->ComputeUsage);

            UI::EndPropertyTable();
        }
    }
}
