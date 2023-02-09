#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Core/UUID.h"

#include "Athena/Renderer/EditorCamera.h"
#include "Athena/Renderer/Environment.h"
#include "Athena/Renderer/Material.h"

#include "Athena/Scene/Components.h" 

#ifdef _MSC_VER
	#pragma warning(push, 0)
#endif

#include <entt/entt.h>

#ifdef _MSC_VER
	#pragma warning(pop)
#endif

#include <unordered_map>


class b2World;

namespace Athena
{
	class ATHENA_API Environment;


	class ATHENA_API Scene
	{
	public:
		friend class Entity;
		friend class ScriptEntity;
		friend class SceneHierarchyPanel;
		friend class ATHENA_API SceneSerializer;

	public:
		Scene(const String& name = "UnNamed");
		~Scene();

		Entity CreateEntity(const String& name, UUID id);
		Entity CreateEntity(const String& name = "UnNamed");
		void DestroyEntity(Entity entity);

		Entity GetEntityByUUID(UUID uuid);
		Entity FindEntityByName(const String& name);

		void OnUpdateEditor(Time frameTime, EditorCamera& camera); 
		void OnUpdateRuntime(Time frameTime);
		void OnUpdateSimulation(Time frameTime, EditorCamera& camera);

		void OnRuntimeStart();
		void OnSimulationStart();

		void OnViewportResize(uint32 width, uint32 height);

		void SetSceneName(const String& name) { m_Name = name; }
		const String& GetSceneName() const { return m_Name; };

		Entity GetPrimaryCameraEntity();

		Ref<Environment> GetEnvironment() { return m_Environment; }
		void SetEnvironment(const Ref<Environment>& env) { m_Environment = env; }

		template <typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

	private:
		void OnPhysics2DStart();
		void UpdatePhysics(Time frameTime);

		template <typename T>
		void OnComponentAdd(Entity entity, T& component);

		template <typename T>
		void OnComponentRemove(Entity entity, T& component);

	private:
		String m_Name;

		entt::registry m_Registry;
		std::unordered_map<UUID, entt::entity> m_EntityMap;

		std::unique_ptr<b2World> m_PhysicsWorld;
		Ref<Environment> m_Environment;

		uint32 m_ViewportWidth = 0, m_ViewportHeight = 0;
	};
}
