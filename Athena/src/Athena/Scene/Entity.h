#pragma once

#include "Scene.h"

#include <entt.h>

namespace Athena
{
	class Entity
	{
	public:
		
	private:
		entt::entity m_EntityHandle{ entt::null };
		Scene* m_Scene = nullptr;
	};
}
