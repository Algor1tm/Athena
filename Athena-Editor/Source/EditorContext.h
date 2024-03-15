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

	struct EditorSettings
	{
		bool GizmosLocalTransform = true;
		float CameraSpeedLevel = 0.3f;
		bool ShowRendererIcons = true;
		float RendererIconsScale = 1.f;
		bool ShowPhysicsColliders = false;
		bool ReloadScriptsOnStart = false;
	};

	struct EditorContext : public RefCounted
	{
		Entity SelectedEntity;
		EditorSettings EditorSettings;
		Ref<Scene> ActiveScene;
		SceneState SceneState;
	};
}
