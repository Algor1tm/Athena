#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Camera.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Pipeline.h"
#include "Athena/Renderer/Material.h"


namespace Athena
{
	struct QuadVertex
	{
		Vector3 Position;
		LinearColor Color;
		Vector2 TexCoords;
		uint32 TexIndex;
	};

	struct CircleVertex
	{
		Vector3 WorldPosition;
		Vector3 LocalPosition;
		LinearColor Color;
		float Thickness;
		float Fade;
	};

	struct LineVertex
	{
		Vector3 Position;
		LinearColor Color;
	};

	struct DrawCall2D
	{
		uint32 VertexBufferIndex;
		uint32 VertexCount;
	};

	class ATHENA_API SceneRenderer2D : public RefCounted
	{
	public:
		static Ref<SceneRenderer2D> Create(const Ref<RenderPass>& renderPass);
		~SceneRenderer2D();

		void Init(const Ref<RenderPass>& renderPass);
		void Shutdown();

		void OnViewportResize(uint32 width, uint32 height);

		void BeginScene(const Matrix4& viewMatrix, const Matrix4& projectionMatrix);
		void EndScene();

		void DrawQuad(Vector2 position, Vector2 size, const LinearColor& color = LinearColor::White);
		void DrawQuad(Vector3 position, Vector2 size, const LinearColor& color = LinearColor::White);
		void DrawQuad(Vector2 position, Vector2 size, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);
		void DrawQuad(Vector3 position, Vector2 size, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);

		void DrawRotatedQuad(Vector2 position, Vector2 size, float rotation, const LinearColor& color = LinearColor::White);
		void DrawRotatedQuad(Vector3 position, Vector2 size, float rotation, const LinearColor& color = LinearColor::White);
		void DrawRotatedQuad(Vector2 position, Vector2 size, float rotation, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);
		void DrawRotatedQuad(Vector3 position, Vector2 size, float rotation, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);

		void DrawQuad(const Matrix4& transform, const LinearColor& color = LinearColor::White);
		void DrawQuad(const Matrix4& transform, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);

		void DrawScreenSpaceQuad(const Vector3& position, Vector2 size, const LinearColor& color = LinearColor::White);
		void DrawScreenSpaceQuad(const Vector3& position, Vector2 size, const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f);

		void DrawCircle(const Matrix4& transform, const LinearColor& color = LinearColor::White, float thickness = 1.f, float fade = 0.005f);

		void DrawLine(const Vector3& p0, const Vector3& p1, const LinearColor& color = LinearColor::White);

		void DrawRect(const Vector3& position, const Vector2& size, const LinearColor& color = LinearColor::White);
		void DrawRect(const Matrix4& transform, const LinearColor& color = LinearColor::White);

		void SetLineWidth(float width);
		float GetLineWidth();

	private:
		void FlushQuads();
		void FlushCircles();
		void FlushLines();

	private:
		// Max Geometry per batch
		const uint32 s_MaxQuads = 500;
		const uint32 s_MaxQuadVertices = s_MaxQuads * 4;
		const uint32 s_MaxCircles = 300;
		const uint32 s_MaxCircleVertices = s_MaxCircles * 4;
		const uint32 s_MaxLines = 300;
		const uint32 s_MaxLineVertices = s_MaxLines * 2;
		const uint32 s_MaxIndices = Math::Max(s_MaxQuads, s_MaxCircles) * 6;

		static const uint32 s_MaxTextureSlots = 32;   // TODO: RenderCaps

	private:
		Ref<RenderCommandBuffer> m_RenderCommandBuffer;
		Ref<IndexBuffer> m_IndexBuffer;
		Matrix4 m_InverseViewCamera;

		Ref<Pipeline> m_QuadPipeline;
		std::vector<Ref<Material>> m_QuadMaterials;
		std::vector<std::vector<Ref<VertexBuffer>>> m_QuadVertexBuffers;
		std::vector<DrawCall2D> m_QuadDrawList;
		uint32 m_QuadIndexCount = 0;
		uint32 m_QuadVertexBufferIndex = 0;
		QuadVertex* m_QuadVertexBufferBase = nullptr;
		QuadVertex* m_QuadVertexBufferPointer = nullptr;

		std::array<Ref<Texture2D>, s_MaxTextureSlots> m_TextureSlots;
		uint32 m_TextureSlotIndex = 1; // 0 - white texture


		Ref<Pipeline> m_CirclePipeline;
		Ref<Material> m_CircleMaterial;
		std::vector<std::vector<Ref<VertexBuffer>>> m_CircleVertexBuffers;
		std::vector<DrawCall2D> m_CircleDrawList;
		uint32 m_CircleIndexCount = 0;
		uint32 m_CircleVertexBufferIndex = 0;
		CircleVertex* m_CircleVertexBufferBase = nullptr;
		CircleVertex* m_CircleVertexBufferPointer = nullptr;


		Ref<Pipeline> m_LinePipeline;
		Ref<Material> m_LineMaterial;
		std::vector<std::vector<Ref<VertexBuffer>>> m_LineVertexBuffers;
		std::vector<DrawCall2D> m_LineDrawList;
		uint32 m_LineVertexCount = 0;
		uint32 m_LineVertexBufferIndex = 0;
		LineVertex* m_LineVertexBufferBase = nullptr;
		LineVertex* m_LineVertexBufferPointer = nullptr;

		Vector4 m_QuadVertexPositions[4];
	};
}
