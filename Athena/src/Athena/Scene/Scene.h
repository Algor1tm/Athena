#pragma once

#include "Athena/Core/Time.h"

#include <entt.h>


namespace Athena
{
	class Scene
	{
	public:
		friend class Entity;

		Scene();
		~Scene();

		Entity CreateEntity(std::string_view name = "UnNamed");
		void OnUpdate(Time frameTime);

	private:

		entt::registry m_Registry;
	};
}
