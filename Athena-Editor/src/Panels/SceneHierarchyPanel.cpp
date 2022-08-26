#include "SceneHierarchyPanel.h"
#include "Athena/Scene/Components.h"
#include "UI/Controllers.h"

#include <ImGui/imgui.h>


namespace Athena
{
	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
		m_SelectionContext = Entity{};
	}

	void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{
		m_SelectionContext = entity;
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
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
		}
		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		const auto& tag = entity.GetComponent<TagComponent>().Tag;

		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0);
		flags |= ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
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

	void SceneHierarchyPanel::DrawAllComponents(Entity entity)
	{
		DrawComponent<TagComponent>(entity, "Tag", [](TagComponent& tagComponent)
			{
				String& tag = tagComponent.Tag;

				static char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				strcpy_s(buffer, tag.c_str());
				if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
					tag = String(buffer);
			});
		
		DrawComponent<TransformComponent>(entity, "Transform", [](TransformComponent& transform)
			{
				UI::DrawVec3Controller("Position", transform.Translation, 0.0f);
				Vector3 degrees = Math::Degrees(transform.Rotation);
				UI::DrawVec3Controller("Rotation", degrees, 0.0f);
				transform.Rotation = Math::Radians(degrees);
				UI::DrawVec3Controller("Scale", transform.Scale, 1.0f);

				ImGui::Spacing();
			});

		DrawComponent<SpriteComponent>(entity, "Sprite", [](SpriteComponent& sprite)
			{
				if (sprite.Texture.GetNativeTexture() != nullptr)
				{
					UI::DrawController("Tint", 60, [&sprite]() { ImGui::ColorEdit4("##Tint", sprite.Color.Data()); });
					UI::DrawController("Tiling", 60, [&sprite]() { ImGui::DragFloat("##Tiling", &sprite.TilingFactor, 0.05f); });
				}
				else
				{
					UI::DrawController("Color", 60, [&sprite]() { ImGui::ColorEdit4("##Color", sprite.Color.Data()); });
				}
			});

		DrawComponent<CameraComponent>(entity, "Camera", [](CameraComponent& cameraComponent)
			{
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
				ImGui::Checkbox("FixedAspectRatio", &cameraComponent.FixedAspectRatio);
			});

		ImGui::Separator();

		if (ImGui::Button("Add"))
			ImGui::OpenPopup("Add");

		if (ImGui::BeginPopup("Add"))
		{
			if (!entity.HasComponent<CameraComponent>() && ImGui::MenuItem("Camera"))
			{
				m_SelectionContext.AddComponent<CameraComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (!entity.HasComponent<SpriteComponent>() && ImGui::MenuItem("Sprite"))
			{
				m_SelectionContext.AddComponent<SpriteComponent>();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
}
