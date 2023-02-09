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

		static void Render(Scene* scene, const Matrix4& viewMatrix, const Matrix4& projectionMatrix);
		static void RenderEditorScene(Scene* scene, const EditorCamera& camera);
	};
}
