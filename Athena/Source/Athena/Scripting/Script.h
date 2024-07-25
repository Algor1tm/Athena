#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Scene/Entity.h"
#include "Athena/Scripting/ScriptFields.h"


namespace Athena
{
	class Script
	{
	public:
		Script() = default;
		virtual ~Script() = default;

		template <typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			return m_Entity.AddComponent<T>(std::forward<Args>(args)...);
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			return m_Entity.AddOrReplaceComponent<T>(std::forward<Args>(args)...);
		}

		template <typename T>
		void RemoveComponent()
		{
			m_Entity.RemoveComponent<T>();
		}

		template <typename T>
		T& GetComponent()
		{
			return m_Entity.GetComponent<T>();;
		}

		template <typename T>
		const T& GetComponent() const
		{
			return m_Entity.GetComponent<T>();;
		}

		template <typename T>
		bool HasComponent() const
		{
			return m_Entity.HasComponent<T>();;
		}

		Scene* GetScene() const { return m_Scene; }

	public:
		virtual void OnCreate() {};
		virtual void OnUpdate(Time frameTime) {};
		virtual void GetFieldsDescription(ScriptFieldMap* outFieldsDesc) {};

	private:
		friend class ATHENA_API ScriptEngine;
		friend class ATHENA_API ScriptClass;

		void Initialize(Scene* scene, Entity entity)
		{
			m_Scene = scene;
			m_Entity = entity;
		}

	private:
		Scene* m_Scene;
		Entity m_Entity;
	};


	using ScriptFunc_Instantiate = void (*)(Script** outScript);
	using ScriptFunc_OnCreate = void (*)(Script* _this);
	using ScriptFunc_OnUpdate = void (*)(Script* _this, float frameTime);
	using ScriptFunc_GetFieldsDescription = void (*)(Script* _this, ScriptFieldMap* outFields);

#ifdef ATN_PLATFORM_WINDOWS
	#define SCRIPT_API __declspec(dllexport)
#else
	#define SCRIPT_API
#endif // End of linking detection
}
