#pragma once

#include "Athena/Core/Color.h"

#include "Athena/Renderer/Camera.h"
#include "Athena/Renderer/OrthographicCamera.h"
#include "Athena/Renderer/EditorCamera.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	class ATHENA_API Renderer2D
	{
	public:
		Renderer2D() = delete;

		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera, const Matrix4& transform);
		static void BeginScene(const OrthographicCamera& camera);
		static void BeginScene(const EditorCamera& camera);
		static void EndScene();
		static void Flush();

		static void DrawQuad(const Vector2& position, const Vector2& size, const LinearColor& color = LinearColor::White);
		static void DrawQuad(const Vector3& position, const Vector2& size, const LinearColor& color = LinearColor::White);
		static void DrawQuad(const Vector2& position, const Vector2& size, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);
		static void DrawQuad(const Vector3& position, const Vector2& size, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);

		static void DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const LinearColor& color = LinearColor::White);
		static void DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const LinearColor& color = LinearColor::White);
		static void DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);
		static void DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);

		static void DrawQuad(const Matrix4& transform, const LinearColor& color = LinearColor::White, int entityID = -1);
		static void DrawQuad(const Matrix4& transform, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f, int entityID = -1);

		static void DrawCircle(const Matrix4& transform, const LinearColor& color = LinearColor::White, float thickness = 1.f, float fade = 0.005f, int entityID = -1);

		static void DrawLine(const Vector3& p0, const Vector3& p1, const LinearColor& color = LinearColor::White, float width = 1.f, int entityID = -1);

		static void DrawRect(const Vector3& position, const Vector2& size, const LinearColor& color = LinearColor::White, float lineWidth = 1.f, int entityID = -1);
		static void DrawRect(const Matrix4& transform, const LinearColor& color = LinearColor::White, float lineWidth = 1.f, int entityID = -1);

		// Stats
		struct Statistics
		{
			uint32 DrawCalls = 0;
			uint32 QuadCount = 0;
			uint32 CircleCount = 0;
			uint32 LineCount = 0;

			uint32 GetTotalVertexCount() { return QuadCount * 4 + CircleCount * 4 + LineCount * 2; }
			uint32 GetTotalIndexCount() { return QuadCount * 6 + CircleCount * 6 + LineCount * 2; }
		};

		static void ResetStats();
		static Statistics GetStats();

	private:
		static void StartBatch();
		static void NextBatch();
	};
}
