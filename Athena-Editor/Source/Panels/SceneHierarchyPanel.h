#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"
#include "Athena/Scene/Entity.h"

#include "Athena/Renderer/Texture.h"

#include "Panel.h"

#include <string_view>


namespace Athena
{
	class SceneHierarchyPanel : public Panel
	{
	public:
		SceneHierarchyPanel(std::string_view name, const Ref<Scene>& context);

		void SetContext(const Ref<Scene>& context);

		virtual void OnImGuiRender() override;

		void SetSelectedEntity(Entity entity);
		Entity GetSelectedEntity() const { return m_SelectionContext; }

	private:
		void DrawEntityNode(Entity entity);
		void DrawAllComponents(Entity entity);

		template <typename Component, typename Func>
		void DrawComponent(Entity entity, std::string_view name, Func uiFunction);

		template <typename Component>
		void DrawAddComponentEntry(Entity entity, std::string_view name);

	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;

		bool m_EditTagComponent = false;
	};



	template <typename Component, typename Func>
	void SceneHierarchyPanel::DrawComponent(Entity entity, std::string_view name, Func uiFunction)
	{
		ImGuiTreeNodeFlags flags =
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_AllowItemOverlap |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_Framed |
			ImGuiTreeNodeFlags_FramePadding;

		if (entity.HasComponent<Component>())
		{
			ImVec2 regionAvail = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4, 6 });

			float lineHeight = ImGui::GetFrameHeight();

			bool open = ImGui::TreeNodeEx((void*)typeid(Component).hash_code(), flags, name.data());

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
				ImGui::TreePop();
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
