#pragma once

#include "Scene.h"

#include <entt.h>


namespace Athena
{
	class Entity
	{
	public:
		Entity() = default;

		Entity(entt::entity handle, Scene* scene)
			: m_EntityHandle(handle), m_Scene(scene) {}

		template <typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			ATN_CORE_ASSERT(!HasComponent<T>(), "Entity already has this component!");

			T& component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
			m_Scene->OnComponentAdd<T>(*this, component);

			return component;
		}

		template <typename T>
		void RemoveComponent()
		{
			ATN_CORE_ASSERT(HasComponent<T>(), "Entity does not have this component!");

			m_Scene->OnComponentRemove<T>(*this);
			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		template <typename T>
		T& GetComponent()
		{
			ATN_CORE_ASSERT(HasComponent<T>(), "Entity does not have this component!");

			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template <typename T>
		const T& GetComponent() const
		{
			ATN_CORE_ASSERT(HasComponent<T>(), "Entity does not have this component!");

			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template <typename T>
		bool HasComponent() const
		{
			return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
		}

		operator bool() const { return m_EntityHandle != entt::null; }
		operator uint32() const  { return (uint32)m_EntityHandle; }
		operator entt::entity() const { return m_EntityHandle; }

		bool operator==(const Entity entity) const 
		{ 
			return m_EntityHandle == entity.m_EntityHandle && m_Scene == entity.m_Scene; 
		}

		bool operator!=(const Entity entity) const
		{
			return m_EntityHandle != entity.m_EntityHandle || m_Scene != entity.m_Scene;
		}

	private:
		entt::entity m_EntityHandle = entt::null;
		Scene* m_Scene = nullptr;
	};
}
