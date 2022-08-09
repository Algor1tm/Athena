#pragma once

#include <entt.h>


namespace Athena
{
	class Scene
	{
	public:
		Scene();
		~Scene();

	private:
		entt::registry m_Registry;
	};
}
