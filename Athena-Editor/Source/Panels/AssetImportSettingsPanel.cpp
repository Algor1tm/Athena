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
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureFilter), (uint32)TextureFilter::NEAREST, "Nearest");
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureFilter), (uint32)TextureFilter::LINEAR, "Linear");
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureFilter), (uint32)TextureFilter::TRILINEAR, "Trilinear");

        UI::RegisterEnum(ATN_STRINGIFY_MACRO(TextureWrap));
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureWrap), (uint32)TextureWrap::REPEAT, "Repeat");
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureWrap), (uint32)TextureWrap::CLAMP_TO_EDGE, "Clamp to edge");
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureWrap), (uint32)TextureWrap::CLAMP_TO_BORDER, "Clamp to border");
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureWrap), (uint32)TextureWrap::MIRRORED_REPEAT, "Mirrored repeat");
        UI::EnumAdd(ATN_STRINGIFY_MACRO(TextureWrap), (uint32)TextureWrap::MIRRORED_CLAMP_TO_EDGE, "Mirrored clamp to edge");

        UI::RegisterEnum("HDR Format");
        UI::EnumAdd("HDR Format", (uint32)Format::R11G11B10F, "R11G11B10F");
        UI::EnumAdd("HDR Format", (uint32)Format::RGBA16F, "RGBA16F");
        UI::EnumAdd("HDR Format", (uint32)Format::RGBA32F, "RGBA32F");
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

        switch (type)
        {
        case AssetType::Mesh:           DrawMeshImportSettings(); break;
        case AssetType::Texture:        DrawTextureImportSettings(); break;
        case AssetType::EnvironmentMap: DrawEnvMapImportSettings(); break;
        }

        UI::PushFont(UI::Fonts::Bold);
        ImGui::Text(AssetManager::GetAssetFilePath(m_AssetHandle).string().data());
        UI::PopFont();

        if (ImGui::Button("Save"))
        {
            OnSave();
            //OnClose();
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset"))
        {
            OnReset();
            //OnClose();
        }

        ImGui::SameLine();

        if (ImGui::Button("Close"))
        {
            OnClose();
        }

        ImGui::End();
    }

    void AssetImportSettingsPanel::OnOpen(AssetHandle assetHandle)
    {
        if (!AssetManager::IsAssetHandleValid(assetHandle) || !Project::GetEditorAssetManager()->HasImportSettings(Project::GetEditorAssetManager()->GetAssetType(assetHandle)))
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
        Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(m_AssetHandle);

        if (mesh && UI::TreeNode("MESH INFO"))
        {
            ImGui::Text("Materials Count: %d", mesh->GetMaterialTable().size());
            ImGui::Text("Animations Count: %d", mesh->GetAnimations().size());
            ImGui::Spacing();
            ImGui::Text("Vertices: %d", mesh->GetVertexBuffer()->GetSize() / sizeof(MeshVertex));
            ImGui::Text("VertexBuffer Size: %s", Utils::MemoryBytesToString(mesh->GetVertexBuffer()->GetSize()).c_str());
            ImGui::Text("Indices: %d", mesh->GetIndexBuffer()->GetSize());
            ImGui::Text("IndexBuffer Size: %s", Utils::MemoryBytesToString(mesh->GetIndexBuffer()->GetSize() * sizeof(uint32)).c_str());

            uint32 boneInfluenceSize = mesh->IsRigged() ? mesh->GetBonesInfluenceBuffer()->GetSize() : 0;
            ImGui::Text("BonesInfluenceBuffer Size: %s", Utils::MemoryBytesToString(boneInfluenceSize).c_str());

            ImGui::Spacing();

            uint32 totalGPUMemory = boneInfluenceSize + mesh->GetVertexBuffer()->GetSize() + mesh->GetIndexBuffer()->GetSize() * sizeof(uint32);
            ImGui::Text("Total GPU Memory: %s", Utils::MemoryBytesToString(totalGPUMemory).c_str());

            UI::TreePop();
        }

        if (UI::TreeNode("MESH IMPORT SETTINGS") && UI::BeginPropertyTable())
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

            FilePath assetFilePath = AssetManager::GetAssetFilePath(m_AssetHandle);
            UI::PropertyText("FilePath", assetFilePath.string().data());
            ImGui::SameLine();
            if (ImGui::Button("Open Externally"))
            {
                Platform::OpenFileExternally(AssetFileExtensions::GetImportSettingsPath(AssetManager::GetAssetAbsolutePath(assetFilePath)));
            }

            UI::EndPropertyTable();
            UI::TreePop();
        }
    }

    void AssetImportSettingsPanel::DrawTextureImportSettings()
    {
        Ref<TextureImportSettings> settings = m_ImportSettingsCopy.As<TextureImportSettings>();
        Ref<TextureAsset> textureAsset = AssetManager::GetAsset<TextureAsset>(m_AssetHandle);

        if (textureAsset && UI::TreeNode("TEXTURE INFO"))
        {
            Ref<Texture2D> texture = textureAsset->GetRenderTexture();

            ImGui::Text("Width: %d", texture->GetWidth());
            ImGui::Text("Height: %d", texture->GetHeight());
            ImGui::Text("Mip Count: %d", texture->GetMipLevelsCount());
            ImGui::Spacing();
            ImGui::Text("Total GPU Memory: %s", Utils::MemoryBytesToString(texture->GetTotalGPUMemory()).c_str());
            ImGui::Spacing();

            UI::DrawImage(texture, { 150, 150 });

            UI::TreePop();
        }

        if (UI::TreeNode("TEXTURE IMPORT SETTINGS") && UI::BeginPropertyTable())
        {
            UI::PropertyCheckbox("sRGB", &settings->sRGB);
            UI::PropertyCheckbox("Generate MipMaps", &settings->GenerateMipMaps);
            UI::PropertyEnumCombo("Wrap Mode", ATN_STRINGIFY_MACRO(TextureWrap), &settings->WrapMode);
            UI::PropertyEnumCombo("Filter Mode", ATN_STRINGIFY_MACRO(TextureFilter), &settings->FilterMode);
            UI::PropertySlider("Anisotropy Level", &settings->AnisotropyLevel, 0.f, Renderer::GetRenderCaps().MaxSamplerAnisotropy);
            UI::PropertyCheckbox("Compute Usage", &settings->ComputeUsage);

            FilePath assetFilePath = AssetManager::GetAssetFilePath(m_AssetHandle);
            UI::PropertyText("FilePath", assetFilePath.string().data());
            ImGui::SameLine();
            if (ImGui::Button("Open Externally"))
            {
                Platform::OpenFileExternally(AssetFileExtensions::GetImportSettingsPath(AssetManager::GetAssetAbsolutePath(assetFilePath)));
            }

            UI::EndPropertyTable();
            UI::TreePop();
        }
    }

    void AssetImportSettingsPanel::DrawEnvMapImportSettings()
    {
        Ref<EnvironmentMapImportSettings> settings = m_ImportSettingsCopy.As<EnvironmentMapImportSettings>();
        Ref<EnvironmentMap> envMap = AssetManager::GetAsset<EnvironmentMap>(m_AssetHandle);

        if (envMap && UI::TreeNode("ENVIRONMENT MAP INFO"))
        {
            Ref<TextureCube> environmentTexture = envMap->GetEnvironmentTexture();
            Ref<TextureCube> irradianceTexture = envMap->GetIrradianceTexture();

            ImGui::Text("Environment GPU Memory: %s", Utils::MemoryBytesToString(environmentTexture->GetTotalGPUMemory()).c_str());
            ImGui::Text("Irradiance GPU Memory: %s", Utils::MemoryBytesToString(irradianceTexture->GetTotalGPUMemory()).c_str());
            ImGui::Spacing();
            ImGui::Text("Total GPU Memory: %s", Utils::MemoryBytesToString(environmentTexture->GetTotalGPUMemory() + irradianceTexture->GetTotalGPUMemory()).c_str());
            ImGui::Spacing();

            static int envMapLayer = 0;
            ImGui::SliderInt("Layer", &envMapLayer, 0, 5);

            TextureViewCreateInfo view;
            view.BaseLayer = envMapLayer;
            ImGui::Image(UI::GetTextureID(environmentTexture->GetView(view)), { 300, 300 });
            ImGui::SameLine();
            ImGui::Image(UI::GetTextureID(irradianceTexture->GetView(view)), { 300, 300 });

            UI::TreePop();
        }

        if (UI::TreeNode("ENVIRONMENT MAP IMPORT SETTINGS") && UI::BeginPropertyTable())
        {
            const std::string_view resolutions[] = { "128", "256", "512", "1024", "2048", "4096" };
            String selectedStr = std::to_string(settings->Resolution);
            std::string_view selected = selectedStr.data();

            if (UI::PropertyCombo("Resolution", resolutions, std::size(resolutions), &selected))
            {
	            uint32 resolution = std::atoi(selected.data());
                settings->Resolution = resolution;
            }
            
            UI::PropertyEnumCombo("HDR Format", "HDR Format", &settings->FloatFormat);

            FilePath assetFilePath = AssetManager::GetAssetFilePath(m_AssetHandle);
            UI::PropertyText("FilePath", assetFilePath.string().data());
            ImGui::SameLine();
            if (ImGui::Button("Open Externally"))
            {
                Platform::OpenFileExternally(AssetFileExtensions::GetImportSettingsPath(AssetManager::GetAssetAbsolutePath(assetFilePath)));
            }

            UI::EndPropertyTable();
            UI::TreePop();
        }
    }
}
