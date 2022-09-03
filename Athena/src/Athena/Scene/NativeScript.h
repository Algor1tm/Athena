#pragma once

#include "Entity.h"
#include "Athena/Core/Time.h"
#include "Athena/Events/Event.h"


namespace Athena
{
	class ATHENA_API NativeScript
	{
	public:
		friend class Scene;

		virtual ~NativeScript() = default;


		template <typename T, typename... Args>
		inline T& AddComponent(Args&&... args)
		{
			return m_Entity.AddComponent<T>(std::forward<Args>(args)...);
		}

		template <typename T>
		inline void RemoveComponent()
		{
			m_Entity.RemoveComponent<T>();
		}

		template <typename T>
		inline T& GetComponent()
		{
			return m_Entity.GetComponent<T>();
		}

		template <typename T>
		inline const T& GetComponent() const
		{
			return m_Entity.GetComponent<T>();
		}

		template <typename T>
		inline bool HasComponent() const
		{
			return m_Entity.HasComponent<T>();
		}


	protected:
		virtual void Init() {}
		virtual void Destroy() {}
		virtual void OnUpdate(Time frameTime) {}
		virtual bool OnEvent(Event& event) { return false; }

	private:
		Entity m_Entity;
	};
}
