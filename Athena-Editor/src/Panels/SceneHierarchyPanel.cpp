#include "SceneHierarchyPanel.h"
#include "Athena/Scene/Components.h"

#include <imgui.h>

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
		ImGui::Begin("Scene Hierarchy");

		m_Context->m_Registry.each([=](auto entityID)
			{
				Entity entity(entityID, m_Context.get());

				DrawEntityNode(entity);
			});

		if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			m_SelectionContext = {};

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
		bool opened = ImGui::TreeNodeEx((void*)(uint64)(uint32)entity, flags, tag.data());

		if (ImGui::IsItemClicked())
			m_SelectionContext = entity;

		if (opened)
			ImGui::TreePop();
	}

	void SceneHierarchyPanel::DrawAllComponents(Entity entity)
	{
		DrawComponent<TagComponent>(entity, "Tag", [](Entity entity) 
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

		DrawComponent<TransformComponent>(entity, "Transform", [](Entity entity)
			{
				Matrix4& transform = entity.GetComponent<TransformComponent>().Transform;
				ImGui::DragFloat3("Position", transform[3].Data(), 0.05f);
			});

		DrawComponent<SpriteRendererComponent>(entity, "Sprite", [](Entity entity)
			{
				auto& spriteComponent = entity.GetComponent<SpriteRendererComponent>();

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

		DrawComponent<CameraComponent>(entity, "Camera", [](Entity entity)
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
