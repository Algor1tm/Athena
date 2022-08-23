#pragma once

#include "Athena/Core/Time.h"

#include <entt/entt.h>


namespace Athena
{
	class ATHENA_API Scene
	{
	public:
		friend class Entity;
		friend class SceneHierarchyPanel;

		Scene();
		~Scene();

		Entity CreateEntity(const String& name = "UnNamed");
		void DestroyEntity(Entity entity);

		void OnUpdate(Time frameTime);
		void OnViewportResize(uint32 width, uint32 height);

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


