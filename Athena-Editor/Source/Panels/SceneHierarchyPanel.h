#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"

#include "Athena/Scene/Entity.h"

#include "Panels/Panel.h"

#include <UI/Widgets.h>

#include <string_view>


namespace Athena
{
	class Scene;


	class SceneHierarchyPanel : public Panel
	{
	public:
		SceneHierarchyPanel(std::string_view name, const Ref<Scene>& context);

		void SetContext(const Ref<Scene>& context);

		virtual void OnImGuiRender() override;

		void SetSelectedEntity(Entity entity);
		Entity GetSelectedEntity() const { return m_SelectionContext; }

	private:
		void DrawEntitiesHierarchy();
		void DrawEntityNode(Entity entity, bool open = false);
		void DrawAllComponents(Entity entity);
		void DrawEnvironmentEditor(const Ref<Environment>& environment);

		void DrawMaterialsEditor();
		void DrawMaterialProperty(Ref<Material> mat, std::string_view name, std::string_view uniformName, MaterialTexture texType, MaterialUniform uniformType);

		template <typename Component, typename Func>
		void DrawComponent(Entity entity, std::string_view name, Func uiFunction);

		template <typename Component>
		void DrawAddComponentEntry(Entity entity, std::string_view name);

	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;

		String m_ActiveMaterial;
		bool m_EditTagComponent = false;
	};



	template <typename Component, typename Func>
	void SceneHierarchyPanel::DrawComponent(Entity entity, std::string_view name, Func uiFunction)
	{
		if (entity.HasComponent<Component>())
		{
			ImVec2 regionAvail = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 3, 5 });

			float lineHeight = ImGui::GetFrameHeight();

			bool open = UI::BeginTreeNode(name.data());

			bool removeComponent = false;

			ImGui::SameLine(regionAvail.x - lineHeight);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
			UI::ShiftCursorY(1.f);
			UI::ShiftCursorX(1.f);
			if (ImGui::Button("...", { lineHeight - 2.f, lineHeight - 2.f }))
				ImGui::OpenPopup("ComponentSettings");
			ImGui::PopStyleVar();

			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("RemoveComponent"))
					removeComponent = true;

				ImGui::EndPopup();
			}

			ImGui::PopStyleVar(2);

			if (open)
			{
				uiFunction(entity.GetComponent<Component>());
				UI::EndTreeNode();
				ImGui::Spacing();
			}

			if (removeComponent)
				entity.RemoveComponent<Component>();
		}
	}

	template <typename Component>
	void SceneHierarchyPanel::DrawAddComponentEntry(Entity entity, std::string_view name)
	{
		if (!entity.HasComponent<Component>() && ImGui::MenuItem(name.data()))
		{
			entity.AddComponent<Component>();
			ImGui::CloseCurrentPopup();
		}
	}
}
