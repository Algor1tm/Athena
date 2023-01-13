#pragma once

#include "RenderCommand.h"
#include "OrthographicCamera.h"
#include "Shader.h"
#include "Mesh.h"


namespace Athena
{
	struct Vertex
	{
		Vector3 Position;
		Vector2 TexCoords;
		Vector3 Normal;
	};

	class ATHENA_API Renderer
	{
	public:
		static void Init(RendererAPI::API graphicsAPI);
		static void Shutdown();

		static void OnWindowResized(uint32 width, uint32 height);

		static void BeginScene(const Matrix4& viewProjection);
		static void EndScene();

		static void Submit(const Ref<Shader>& shader,
			const Ref<StaticMesh>& mesh,
			const Matrix4& transform = Matrix4::Identity());

		static void Clear(const LinearColor& color);
		static Ref<Framebuffer> GetFramebuffer();
		static const BufferLayout& GetVertexBufferLayout();

		static inline RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	};
}
