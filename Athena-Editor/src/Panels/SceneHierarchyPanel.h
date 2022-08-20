#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"
#include "Athena/Scene/Entity.h"

#include <string_view>


namespace Athena
{
	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& context);

		void SetContext(const Ref<Scene>& context);

		void OnImGuiRender();

	private:
		void DrawEntityNode(Entity entity);
		void DrawAllComponents(Entity entity);

		template <typename Component, typename Func>
		void DrawComponent(Entity entity, std::string_view name, int treeNodeFlags, Func function);

	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;
	};



	template <typename Component, typename Func>
	void SceneHierarchyPanel::DrawComponent(Entity entity, std::string_view name, int treeNodeFlags, Func function)
	{
		if (entity.HasComponent<Component>())
		{
			bool removeComponent = false;

			if (ImGui::TreeNodeEx((void*)typeid(Component).hash_code(), treeNodeFlags, name.data()))
			{
				if (treeNodeFlags & ImGuiTreeNodeFlags_AllowItemOverlap)
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 25.f);

					if (ImGui::Button("+"))
						ImGui::OpenPopup("ComponentSettings");


					if (ImGui::BeginPopup("ComponentSettings"))
					{
						if (ImGui::MenuItem("RemoveComponent"))
							removeComponent = true;

						ImGui::EndPopup();
					}
				}

				function(entity);
				ImGui::TreePop();

				ImGui::Spacing();
				ImGui::Separator();
			}

			if (removeComponent)
				entity.RemoveComponent<Component>();
		}
	}
}