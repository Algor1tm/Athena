#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Scene/Scene.h"


namespace Athena
{
	class ATHENA_API SceneRenderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void Render(Scene* scene, const Matrix4& cameraViewProjection, const Vector3& cameraPosition);
		static void RenderEditorScene(Scene* scene, const EditorCamera& camera);
	};
}
