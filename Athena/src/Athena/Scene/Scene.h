#pragma once

#include "Athena/Core/Time.h"

#include <entt.h>


namespace Athena
{
	class ATHENA_API Scene
	{
	public:
		friend class Entity;

		Scene();
		~Scene();

		Entity CreateEntity(std::string_view name = "UnNamed");

		void OnUpdate(Time frameTime);
		void OnViewportResize(uint32 width, uint32 height);

	private:
		entt::registry m_Registry;
		uint32 m_ViewportWidth = 0, m_ViewportHeight = 0;
	};
}
