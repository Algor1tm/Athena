#include "SceneHierarchyPanel.h"
#include "Athena/Scene/Components.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <array>


namespace Athena
{
	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::ShowDemoWindow();

		ImGui::Begin("Scene Hierarchy");

		m_Context->m_Registry.each([=](auto entityID)
			{
				Entity entity(entityID, m_Context.get());

				DrawEntityNode(entity);
			});

		if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			m_SelectionContext = {};

		if (ImGui::BeginPopupContextWindow(0, 1, false))
		{
			if (ImGui::MenuItem("Create Entity"))
				m_Context->CreateEntity();

			ImGui::EndPopup();
		}
		ImGui::End();


		ImGui::Begin("Properties");

		if (m_SelectionContext)
		{
			DrawAllComponents(m_SelectionContext);

			if (ImGui::Button("Add Component"))
				ImGui::OpenPopup("AddComponent");

			if (ImGui::BeginPopup("AddComponent"))
			{
				if (ImGui::MenuItem("Camera"))
				{
					m_SelectionContext.AddComponent<CameraComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Sprite"))
				{
					m_SelectionContext.AddComponent<SpriteComponent>();
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}
		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		const auto& tag = entity.GetComponent<TagComponent>().Tag;

		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0);
		flags |= ImGuiTreeNodeFlags_OpenOnArrow;
		bool opened = ImGui::TreeNodeEx((void*)(uint64)(uint32)entity, flags, tag.data());

		if (ImGui::IsItemClicked())
			m_SelectionContext = entity;

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (opened)
			ImGui::TreePop();

		if (entityDeleted)
		{
			if (m_SelectionContext == entity)
				m_SelectionContext = {};
			m_Context->DestroyEntity(entity);
		}
	}

	static void DrawVec3Control(std::string_view label, Vector3& values, float defaultValues, float columnWidth = 100.f)
	{
		ImGui::PushID(label.data());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.data());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.f;
		ImVec2 buttonSize = { lineHeight + 3, lineHeight };
		
		ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.1f, 0.15f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.9f, 0.2f, 0.2f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.8f, 0.1f, 0.15f, 1.f });
		if (ImGui::Button("X", buttonSize))
			values.x = defaultValues;

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.07f, 0.f, 0.f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PopStyleColor(3);

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.7f, 0.2f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.3f, 0.8f, 0.3f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.2f, 0.7f, 0.2f, 1.f });
		if (ImGui::Button("Y", buttonSize))
			values.y = defaultValues;

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.07f, 0.f, 0.f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PopStyleColor(3);

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.1f, 0.25f, 0.8f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.2f, 0.35f, 0.9f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.1f, 0.25f, 0.8f, 1.f });
		if (ImGui::Button("Z", buttonSize))
			values.z = defaultValues;

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.7f, 0.f, 0.f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleColor(3);

		ImGui::PopStyleVar();
		ImGui::Columns(1);

		ImGui::PopID();
	}

	void SceneHierarchyPanel::DrawAllComponents(Entity entity)
	{
		DrawComponent<TagComponent>(entity, "Tag", ImGuiTreeNodeFlags_DefaultOpen, [](Entity entity)
			{
				String& tag = entity.GetComponent<TagComponent>().Tag;

				static char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				strcpy_s(buffer, tag.c_str());
				if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
				{
					tag = String(buffer);
				}
			});


		DrawComponent<TransformComponent>(entity, "Transform", ImGuiTreeNodeFlags_DefaultOpen, [](Entity entity)
			{
				auto& transform = entity.GetComponent<TransformComponent>();
				DrawVec3Control("Position", transform.Position, 0.0f);
				Vector3 degrees = Math::Degrees(transform.Rotation);
				DrawVec3Control("Rotation", degrees, 0.0f);
				transform.Rotation = Math::Radians(degrees);
				DrawVec3Control("Scale", transform.Scale, 1.0f);
			});

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap;

		DrawComponent<SpriteComponent>(entity, "Sprite", flags, [](Entity entity)
			{
				auto& spriteComponent = entity.GetComponent<SpriteComponent>();

				if (spriteComponent.Texture.GetNativeTexture() != nullptr)
				{
					ImGui::ColorEdit4("Tint", spriteComponent.Color.Data());
					ImGui::DragFloat("Tiling", &spriteComponent.TilingFactor, 0.05f);
				}
				else
				{
					ImGui::ColorEdit4("Color", spriteComponent.Color.Data());
				}
			});

		DrawComponent<CameraComponent>(entity, "Camera", flags, [](Entity entity)
			{
				auto& cameraComponent = entity.GetComponent<CameraComponent>();
				auto& camera = cameraComponent.Camera;

				static auto typeToStr = [](SceneCamera::ProjectionType type) -> std::string_view
				{
					switch (type)
					{
					case SceneCamera::ProjectionType::Orthographic: return "Orthographic";
					case SceneCamera::ProjectionType::Perspective: return "Perspective";
					}

					return "Invalid";
				};

				std::string_view currentProjectionType = typeToStr(camera.GetProjectionType());

				if (ImGui::BeginCombo("Projection", currentProjectionType.data()))
				{
					for (int i = 0; i < 2; ++i)
					{
						const std::string_view typeStr = typeToStr((SceneCamera::ProjectionType)i);
						bool isSelected = currentProjectionType == typeStr;
						if (ImGui::Selectable(typeStr.data(), isSelected))
						{
							currentProjectionType = typeStr;
							camera.SetProjectionType((SceneCamera::ProjectionType)i);
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
				{
					auto perspectiveDesc = camera.GetPerspectiveData();
					bool changed = false;

					float degreesFOV = Math::Degrees(perspectiveDesc.VerticalFOV);
					changed = ImGui::DragFloat("Vertical FOV", &degreesFOV, 0.1f) || changed;
					changed = ImGui::DragFloat("NearClip", &perspectiveDesc.NearClip, 0.1f) || changed;
					changed = ImGui::DragFloat("FarClip", &perspectiveDesc.FarClip, 10.f) || changed;

					if (changed)
					{
						perspectiveDesc.VerticalFOV = Math::Radians(degreesFOV);
						camera.SetPerspectiveData(perspectiveDesc);
					}
				}

				else if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
				{
					auto orthoDesc = camera.GetOrthographicData();
					bool changed = false;
					changed = ImGui::DragFloat("Size", &orthoDesc.Size, 0.1f) || changed;
					changed = ImGui::DragFloat("NearClip", &orthoDesc.NearClip, 0.1f) || changed;
					changed = ImGui::DragFloat("FarClip", &orthoDesc.FarClip, 0.1f) || changed;

					if (changed)
						camera.SetOrthographicData(orthoDesc);
				}

				ImGui::Checkbox("Primary", &cameraComponent.Primary);

			});
	}
}
