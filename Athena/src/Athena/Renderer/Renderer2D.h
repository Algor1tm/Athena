#pragma once

#include "Athena/Renderer/OrthographicCamera.h"
#include "Athena/Renderer/Color.h"


namespace Athena
{
	class Renderer2D
	{
	public:
		Renderer2D() = delete;

		static void Init();
		static void Shutdown();

		static void BeginScene(const OrthographicCamera& camera);
		static void EndScene();

		static void DrawQuad(const Vector2& position, const Vector2& size, const Color& color);
		static void DrawQuad(const Vector3& position, const Vector2& size, const Color& color);
	};
}
