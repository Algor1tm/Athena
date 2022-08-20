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
		{
			m_SelectionContext = entity;
		}

		if (opened)
		{
			ImGui::TreePop();
		}
	}

	void SceneHierarchyPanel::DrawAllComponents(Entity entity)
	{
		DrawTagComponent(entity);
		ImGui::Separator();
		DrawTransformComponent(entity);
		ImGui::Separator();
		DrawCameraComponent(entity);
	}

	void SceneHierarchyPanel::DrawTagComponent(Entity entity)
	{
		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			static char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strcpy_s(buffer, tag.c_str());
			if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
			{
				tag = String(buffer);
			}
		}
	}

	void SceneHierarchyPanel::DrawTransformComponent(Entity entity)
	{
		if (entity.HasComponent<TransformComponent>())
		{
			if (ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Transform"))
			{
				Matrix4& transform = entity.GetComponent<TransformComponent>().Transform;
				ImGui::DragFloat3("Position", transform[3].Data(), 0.05f);

				ImGui::TreePop();
			}
		}
	}

	void SceneHierarchyPanel::DrawCameraComponent(Entity entity)
	{
		if (entity.HasComponent<CameraComponent>())
		{
			if (ImGui::TreeNodeEx((void*)typeid(CameraComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Camera"))
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

					float degreesFOV = Degrees(perspectiveDesc.VerticalFOV);
					changed = ImGui::DragFloat("Vertical FOV", &degreesFOV, 0.1f) || changed;
					changed = ImGui::DragFloat("NearClip", &perspectiveDesc.NearClip, 0.1f) || changed;
					changed = ImGui::DragFloat("FarClip", &perspectiveDesc.FarClip, 10.f) || changed;

					if (changed)
					{
						perspectiveDesc.VerticalFOV = Radians(degreesFOV);
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

				ImGui::TreePop();
			}
		}
	}
}
