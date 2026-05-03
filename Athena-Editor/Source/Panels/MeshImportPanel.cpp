#include "MeshImportPanel.h"
#include "Athena/Asset/AssetManager.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Scene/Components.h"
#include "Athena/UI/UI.h"
#include "Athena/UI/Theme.h"

#include "Panels/PanelManager.h"

#include <ImGui/imgui.h>
#include <queue>


namespace Athena
{
	MeshImportPanel::MeshImportPanel(const Ref<EditorContext>& context)
		: Panel(MESH_IMPORT_PANEL_ID, context)
	{
        m_StaticMeshExt = Project::GetEditorAssetManager()->GetAssetExtensions(AssetType::StaticMesh)[0];
        m_SkeletalMeshExt = Project::GetEditorAssetManager()->GetAssetExtensions(AssetType::SkeletalMesh)[0];
	}

	void MeshImportPanel::OnImGuiRender()
	{
        if (!AssetManager::IsAssetHandleValid(m_MeshSourceHandle) || m_EditorCtx.SceneState != SceneState::Edit)
        {
            OnClose();
            return;
        }

        ImGui::Begin("Create New Mesh");

        if (UI::TreeNode("MESH:") && UI::BeginPropertyTable())
        {
            UI::PropertyRow("Import as", 2 * ImGui::GetFrameHeightWithSpacing());

            if (ImGui::RadioButton("Static Mesh", m_IsStaticMesh))
            {
                m_FilePath.replace_extension(m_StaticMeshExt);
                m_IsStaticMesh = true;
                m_ImportAnimations = false;
            }

            if (ImGui::RadioButton("Skeletal Mesh", !m_IsStaticMesh))
            {
                m_FilePath.replace_extension(m_SkeletalMeshExt);
                m_IsStaticMesh = false;
            }

            UI::PropertyRow("FilePath", ImGui::GetFrameHeight());

            String filepathString = m_FilePath.string();

            if (UI::TextInput("FilePathInput", filepathString))
                m_FilePath = filepathString;
            
            if (m_IsRigged)
            {
                UI::PropertyCheckbox("Import Animations", &m_ImportAnimations);
            }
            else
            {
                ImGui::BeginDisabled();
                bool falseValue = false;
                UI::PropertyCheckbox("Import Animations", &falseValue);
                ImGui::EndDisabled();
            }

            UI::EndPropertyTable();
            UI::TreePop();
        }

        Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(m_MeshSourceHandle);
        bool hasSkeleton = meshSource->GetSkeleton() != nullptr;
        bool hasAnimations = !meshSource->GetAnimations().empty();

        std::string_view label = hasSkeleton ? "SKELETON:" : "NO SKELETON WAS FOUND IN SOURCE";
        if (UI::TreeNode(label.data()) && UI::BeginPropertyTable())
        {
            if (hasSkeleton)
            {
                Ref<Skeleton> skeleton = meshSource->GetSkeleton();

                ImGui::BeginDisabled();
                UI::PropertyCheckbox("Import", &m_ImportAnimations);
                UI::PropertyText("Bones", std::to_string(skeleton->GetBoneCount()).c_str());
                ImGui::EndDisabled();
            }

            UI::EndPropertyTable();
            UI::TreePop();
        }

        label = hasAnimations ? "ANIMATIONS:" : "NO ANIMATIONS WERE FOUND IN SOURCE";
        if (UI::TreeNode(label.data()) && UI::BeginPropertyTable())
        {
            if (hasAnimations)
            {
                const auto& animations = meshSource->GetAnimations();

                ImGui::BeginDisabled();

                for (const auto& animation : animations)
                {
                    UI::PropertyCheckbox(animation->GetName().c_str(), &m_ImportAnimations);
                }

                ImGui::EndDisabled();
            }

            UI::EndPropertyTable();
            UI::TreePop();
        }

        ImGui::PushStyleColor(ImGuiCol_Text, UI::GetTheme().ErrorText);
        bool isValidExt = m_IsStaticMesh ? m_FilePath.extension() == m_StaticMeshExt : m_FilePath.extension() == m_SkeletalMeshExt;
        if (!isValidExt)
            ImGui::Text("Invalid extension!");

        bool isExists = FileSystem::Exists(AssetManager::GetAssetAbsolutePath(m_FilePath));
        if (isExists)
            ImGui::Text("Current file path already exists!");
        ImGui::PopStyleColor();

        bool isValid = isValidExt && !isExists;

        if (ImGui::Button("Create") && isValid)
        {
            if (m_IsStaticMesh)
            {
                Ref<StaticMesh> staticMesh = StaticMesh::Create(m_MeshSourceHandle);
                AssetHandle handle = Project::GetEditorAssetManager()->AddAsset(staticMesh, AssetManager::GetAssetAbsolutePath(m_FilePath));
                CreateStaticMesh(handle);
            }
            else
            {
                Ref<SkeletalMesh> skeletalMesh = SkeletalMesh::Create(m_MeshSourceHandle);
                AssetHandle handle = Project::GetEditorAssetManager()->AddAsset(skeletalMesh, AssetManager::GetAssetAbsolutePath(m_FilePath));

                CreateSkeletalMesh(handle);
            }

            OnClose();
        }

        ImGui::SameLine();

        if (ImGui::Button("Close"))
        {
            OnClose();
        }

        ImGui::End();
	}

    void MeshImportPanel::OnImport(AssetHandle meshSourceHandle)
    {
        if (!AssetManager::IsAssetHandleValid(meshSourceHandle))
            return;

        m_MeshSourceHandle = meshSourceHandle;

        Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(meshSourceHandle);
        m_ImportAnimations = m_IsRigged = meshSource->IsRigged();
        m_IsStaticMesh = !m_IsRigged;

        m_FilePath = AssetManager::GetAssetFilePath(m_MeshSourceHandle);
        m_FilePath.replace_extension(m_StaticMeshExt);
    }

    void MeshImportPanel::OnClose()
    {
        m_MeshSourceHandle = 0;
        m_IsStaticMesh = true;
        m_ImportAnimations = false;
        m_IsRigged = false;
        m_FilePath.clear();

        PanelManager::ClosePanel(MESH_IMPORT_PANEL_ID);
    }

    void MeshImportPanel::CreateStaticMesh(AssetHandle meshHandle)
    {
        Entity entity = m_EditorCtx.ActiveScene->CreateEntity();
        entity.AddComponent<StaticMeshComponent>().MeshHandle = meshHandle;
        entity.GetComponent<TagComponent>().Tag = AssetManager::GetAssetFilePath(meshHandle).stem().string();
        m_EditorCtx.SelectedEntity = entity;
    }

    void MeshImportPanel::CreateSkeletalMesh(AssetHandle meshHandle)
    {
        Ref<SkeletalMesh> skeletalMesh = AssetManager::GetAsset<SkeletalMesh>(meshHandle);

        Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(skeletalMesh->GetMeshSource());
        const MeshNode& rootNode = meshSource->GetRootNode();

        Entity rootEntity = m_EditorCtx.ActiveScene->CreateEntity();
        rootEntity.GetComponent<TagComponent>().Tag = rootNode.Name;

        Vector3 translation, rotation, scale;
        Math::DecomposeTransform(rootNode.LocalTransform, translation, rotation, scale);
        auto& transformComponent = rootEntity.GetComponent<TransformComponent>();
        transformComponent.Translation = translation;
        transformComponent.Rotation = rotation;
        transformComponent.Scale = scale;

        auto& meshComponent = rootEntity.AddComponent<SkeletalMeshComponent>();
        meshComponent.MeshHandle = meshHandle;
        meshComponent.MeshNodeIndex = rootNode.Index;

        CreateEntityHierarchy(meshSource, rootNode, rootEntity, meshHandle);
        m_EditorCtx.SelectedEntity = rootEntity;

        if (m_ImportAnimations)
        {
            AnimationControllerComponent& controller = rootEntity.AddComponent<AnimationControllerComponent>();
            controller.AnimationController = AnimationController::Create(m_MeshSourceHandle);
        }
    }

    void MeshImportPanel::CreateEntityHierarchy(const Ref<MeshSource>& meshSource, const MeshNode& meshNode, Entity entity, AssetHandle meshHandle)
    {
        if (meshNode.Children.empty())
            return;

        entity.AddComponent<ChildComponent>().Children.reserve(meshNode.Children.size());

        for (uint32 childIndex : meshNode.Children)
        {
            const MeshNode& childNode = meshSource->GetMeshNode(childIndex);

            Entity childEntity = m_EditorCtx.ActiveScene->CreateEntity();
            childEntity.AddComponent<ParentComponent>().Parent = entity;
            childEntity.GetComponent<TagComponent>().Tag = childNode.Name;

            Vector3 translation, rotation, scale;
            Math::DecomposeTransform(childNode.LocalTransform, translation, rotation, scale);
            auto& transformComponent = childEntity.GetComponent<TransformComponent>();
            transformComponent.Translation = translation;
            transformComponent.Rotation = rotation;
            transformComponent.Scale = scale;

            auto& meshComponent = childEntity.AddComponent<SkeletalMeshComponent>();
            meshComponent.MeshHandle = meshHandle;
            meshComponent.MeshNodeIndex = childNode.Index;

            entity.GetComponent<ChildComponent>().Children.push_back(childEntity);

            CreateEntityHierarchy(meshSource, childNode, childEntity, meshHandle);
        }
    }
}
