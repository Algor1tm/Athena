#include "MaterialEditorPanel.h"
#include "Athena/Core/FileDialogs.h"
#include "Athena/Renderer/TextureGenerator.h"
#include "Athena/UI/UI.h"

#include "Panels/PanelManager.h"
#include "Panels/ContentBrowserPanel.h"
#include "EditorResources.h"


namespace Athena
{
	static const char* TextureTypeToString(MaterialTextureType type)
	{
		switch (type)
		{
		case MaterialTextureType::Albedo:	 return "Albedo";
		case MaterialTextureType::Normals:	 return "Normals";
		case MaterialTextureType::Roughness: return "Roughness";
		case MaterialTextureType::Metalness: return "Metalness";
		}

		return "";
	}

	MaterialEditorPanel::MaterialEditorPanel(const Ref<EditorContext>& context)
		: Panel(MATERIAL_EDITOR_PANEL_ID, context)
	{

	}

	void MaterialEditorPanel::OnImGuiRender()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 1, 1 });
		ImGui::Begin("Material Editor", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing);

		ImVec2 size = ImGui::GetContentRegionAvail();

		Ref<MaterialAsset> material = m_ActiveMaterial.Get();

		if (material)
		{
			ImGui::Text("Name: "); ImGui::SameLine();
			ImGui::Text(material->GetMaterial()->GetName().data());

			ImGui::Text("Shader: "); ImGui::SameLine();
			ImGui::Text(material->GetMaterial()->GetShader()->GetName().data());

			if (UI::TreeNode("Material") && UI::BeginPropertyTable())
			{
				// Albedo
				{
					RenderTexture(material, MaterialTextureType::Albedo);
					ImGui::SameLine();

					LinearColor albedo = material->GetAlbedo();
					if (ImGui::ColorEdit4("Albedo", albedo.Data(), ImGuiColorEditFlags_NoInputs))
						material->SetAlbedo(albedo);

					float emission = material->GetEmission();
					if (ImGui::DragFloat("Emission", &emission, 1.f, 0.f, 500.f))
						material->SetEmission(emission);
				}

				// Normals
				{
					RenderTexture(material, MaterialTextureType::Normals);
				}

				// Roughness
				{
					RenderTexture(material, MaterialTextureType::Roughness);
					ImGui::SameLine();

					float roughness = material->GetRoughness();

					if (ImGui::SliderFloat("Roughness", &roughness, 0.f, 1.f))
						material->SetRoughness(roughness);
				}

				// Metalness
				{
					RenderTexture(material, MaterialTextureType::Metalness);
					ImGui::SameLine();

					float metalness = material->GetMetalness();
					if (ImGui::SliderFloat("Metalness", &metalness, 0.f, 1.f))
						material->SetMetalness(metalness);
				}

				bool castShadows = material->IsFlagSet(MaterialFlag::CastShadows);
				if (UI::PropertyCheckbox("Cast Shadows", &castShadows))
					material->SetFlag(MaterialFlag::CastShadows, castShadows);

				UI::EndPropertyTable();
				UI::TreePop();
				ImGui::Spacing();
			}
		}

		ImGui::SetCursorPos(ImVec2(1, ImGui::GetFrameHeight() + 1));
		ImGui::InvisibleButton("##DragDropRegion", ImVec2(size.x, size.y));

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				CBDragDropPayload* cbPayload = (CBDragDropPayload*)payload->Data;

				if (cbPayload->AssetType == AssetType::Material)
					m_ActiveMaterial = cbPayload->AssetHandle;
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void MaterialEditorPanel::RenderTexture(const Ref<MaterialAsset>& material, MaterialTextureType type) const
	{
		float imageSize = 45.f * ImGui::GetIO().FontGlobalScale;

		Ref<Texture2D> texture = material->GetTexture(type).Get();
		Ref<Texture2D> displayTexture = texture ? texture : EditorResources::GetIcon("Empty Texture");

		if (UI::PropertyImage(TextureTypeToString(type), displayTexture, {imageSize, imageSize,}))
		{
			String textureExts = Project::GetEditorAssetManager()->GetAssetExtensions(AssetType::Texture2D);
			FilePath path = FileDialogs::OpenFile("Select Texture", { "Texture files", textureExts }, Project::GetAssetDirectory());
			AssetHandle handle = Project::GetEditorAssetManager()->GetAssetHandleFromFilePath(path);

			if (Project::GetEditorAssetManager()->IsAssetHandleValid(handle))
			{
				material->SetTexture(type, handle);
				material->EnableTexture(type, true);
			}
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				CBDragDropPayload* cbPayload = (CBDragDropPayload*)payload->Data;

				if (cbPayload->AssetType == AssetType::Texture2D)
				{
					material->SetTexture(type, cbPayload->AssetHandle);
					material->EnableTexture(type, true);
				}
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();

		bool useTexture = material->IsEnabledTexture(type);
		if (ImGui::Checkbox("Use", &useTexture))
			material->EnableTexture(type, useTexture);
	}

	void MaterialEditorPanel::SetActiveMaterial(const AssetHandleRef<MaterialAsset>& material)
	{
		m_ActiveMaterial = material;
		ImGui::SetWindowFocus("Material Editor");
	}
}
