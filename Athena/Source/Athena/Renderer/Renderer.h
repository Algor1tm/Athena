#pragma once

#include "RenderCommand.h"
#include "OrthographicCamera.h"
#include "Shader.h"


namespace Athena
{
	class ATHENA_API Renderer
	{
	public:
		Renderer() = delete;
		static void Init();
		static void Shutdown();

		static void OnWindowResized(uint32 width, uint32 height);

		static void BeginScene(const OrthographicCamera& Camera);
		static void EndScene();

		static void Submit(const Ref<Shader>& shader,
			const Ref<VertexBuffer>& vertexBuffer,
			const Matrix4& transform = Matrix4::Identity());

		static inline RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

	private:
		struct SceneData
		{
			Matrix4 ViewProjectionMatrix = Matrix4::Identity();
		};

		static SceneData* m_SceneData;
	};
}
