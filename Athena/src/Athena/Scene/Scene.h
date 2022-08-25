#pragma once

#include "Athena/Core/Time.h"
#include "Athena/Renderer/EditorCamera.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#include <entt/entt.h>
#pragma warning(pop)
#else
#include <entt/entt.h>
#endif


namespace Athena
{
	class ATHENA_API Scene
	{
	public:
		friend class Entity;
		friend class SceneHierarchyPanel;
		friend class SceneSerializer;

		Scene();
		~Scene();

		Entity CreateEntity(const String& name = "UnNamed");
		void DestroyEntity(Entity entity);

		void OnUpdateEditor(Time frameTime, EditorCamera& camera); 
		void OnUpdateRuntime(Time frameTime);
		void OnViewportResize(uint32 width, uint32 height);

		Entity GetPrimaryCameraEntity();

	private:
		template <typename T>
		void OnComponentAdd(Entity entity, T& component);

		template <typename T>
		void OnComponentRemove(Entity entity);

	private:
		entt::registry m_Registry;
		uint32 m_ViewportWidth = 0, m_ViewportHeight = 0;
	};


	template <typename T>
	void Scene::OnComponentAdd(Entity entity, T& component) {}

	template<typename T>
	void Scene::OnComponentRemove(Entity entity) {}
}
