#pragma once

#include "RenderCommand.h"
#include "OrthographicCamera.h"
#include "Shader.h"


namespace Athena
{
	class ATHENA_API Renderer
	{
	public:
		static void Init(RendererAPI::API graphicsAPI);
		static void Shutdown();

		static void OnWindowResized(uint32 width, uint32 height);

		static void BeginScene(const Matrix4& viewProjection);
		static void EndScene();

		static void Submit(const Ref<Shader>& shader,
			const Ref<VertexBuffer>& vertexBuffer,
			const Matrix4& transform = Matrix4::Identity());

		static void Clear(const LinearColor& color);
		static Ref<Framebuffer> GetFramebuffer();

		static inline RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	};
}
