#pragma once

#include "Athena/Renderer/OrthographicCamera.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/SubTexture2D.h"


namespace Athena
{
	class ATHENA_API Renderer2D
	{
	public:
		Renderer2D() = delete;

		static void Init();
		static void Shutdown();

		static void BeginScene(const OrthographicCamera& camera);
		static void EndScene();
		static void Flush();

		static void DrawQuad(const Vector2& position, const Vector2& size, const Color& color);
		static void DrawQuad(const Vector3& position, const Vector2& size, const Color& color);
		static void DrawQuad(const Vector2& position, const Vector2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.f, const Color& tint = Color::White);
		static void DrawQuad(const Vector3& position, const Vector2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.f, const Color& tint = Color::White);
		static void DrawQuad(const Vector2& position, const Vector2& size, const Ref<SubTexture2D>& subtexture, float tilingFactor = 1.f, const Color& tint = Color::White);
		static void DrawQuad(const Vector3& position, const Vector2& size, const Ref<SubTexture2D>& subtexture, float tilingFactor = 1.f, const Color& tint = Color::White);

		static void DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Color& color);
		static void DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const Color& color);
		static void DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.f, const Color& tint = Color::White);
		static void DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.f, const Color& tint = Color::White);
		static void DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Ref<SubTexture2D>& subtexture, float tilingFactor = 1.f, const Color& tint = Color::White);
		static void DrawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const Ref<SubTexture2D>& subtexture, float tilingFactor = 1.f, const Color& tint = Color::White);


		// Stats
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;

			uint32_t GetTotalVertexCount() { return QuadCount * 4; }
			uint32_t GetTotalIndexCount() { return QuadCount * 6; }
		};

		static void ResetStats();
		static Statistics GetStats();

	private:
		static void StartBatch();
		static void NextBatch();
	};
}
