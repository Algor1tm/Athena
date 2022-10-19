#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Scene/Entity.h"


namespace Athena
{
	class ATHENA_API ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnUpdateEntity(Entity entity, float frameTime);
	};
}
