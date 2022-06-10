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

		static void BeginScene(OrthographicCamera& Camera);
		static void EndScene();

		static void Submit(const std::shared_ptr<Shader>& shader, 
			const std::shared_ptr<VertexArray>& vertexArray, 
			const Matrix4& transform = Matrix4());

		static inline RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

	private:
		struct SceneData
		{
			Matrix4 ViewProjectionMatrix;
		};

		static SceneData* m_SceneData;
	};
}
