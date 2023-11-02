#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Scene/Scene.h"
#include "Athena/Scene/Entity.h"


namespace Athena
{
	enum class SceneState
	{
		Edit = 0, Play = 1, Simulation = 2
	};

	struct EditorContext : public RefCounted
	{
		Entity SelectedEntity;
		Ref<Scene> ActiveScene;
		SceneState SceneState;
	};
}
