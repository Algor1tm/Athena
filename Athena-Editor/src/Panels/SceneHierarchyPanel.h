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
		void DrawComponent(Entity entity, std::string_view name, Func function)
		{
			if (entity.HasComponent<Component>())
			{
				if (ImGui::TreeNodeEx((void*)typeid(Component).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, name.data()))
				{
					function(entity);
					ImGui::TreePop();
				}

				ImGui::Separator();
			}
		}


	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;
	};
}