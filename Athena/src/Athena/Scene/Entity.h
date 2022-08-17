#pragma once

#include "Scene.h"

#include <entt.h>


namespace Athena
{
	class Entity
	{
	public:
		inline Entity() = default;

		inline Entity(entt::entity handle, Scene* scene)
			: m_EntityHandle(handle), m_Scene(scene) {}

		template <typename T, typename... Args>
		inline T& AddComponent(Args&&... args)
		{
			ATN_CORE_ASSERT(!HasComponent<T>(), "Entity already has this component!");

			return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template <typename T>
		inline void RemoveComponent()
		{
			ATN_CORE_ASSERT(HasComponent<T>(), "Entity does not have this component!");

			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		template <typename T>
		inline T& GetComponent()
		{
			ATN_CORE_ASSERT(HasComponent<T>(), "Entity does not have this component!");

			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template <typename T>
		inline const T& GetComponent() const
		{
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template <typename T>
		inline bool HasComponent() const
		{
			return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
		}

		operator bool() { return m_EntityHandle != entt::null; }

	private:
		entt::entity m_EntityHandle = entt::null;
		Scene* m_Scene = nullptr;
	};
}
