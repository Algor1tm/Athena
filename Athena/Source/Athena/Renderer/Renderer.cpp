#include "atnpch.h"
#include "Renderer.h"

#include "Athena/Renderer/Renderer2D.h"


namespace Athena
{
	Renderer::SceneData* Renderer::m_SceneData = new Renderer::SceneData();

	void Renderer::Init()
	{
		RenderCommand::Init();
		Renderer2D::Init();
	}

	void Renderer::Shutdown()
	{
		Renderer2D::Shutdown();
	}
	
	void Renderer::OnWindowResized(uint32 width, uint32 height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::BeginScene(const OrthographicCamera& Camera)
	{
		m_SceneData->ViewProjectionMatrix = Camera.GetViewProjectionMatrix();
	}
	
	void Renderer::EndScene()
	{

	}
		
	void Renderer::Submit(const Ref<Shader>& shader,
		const Ref<VertexBuffer>& vertexBuffer,
		const Matrix4& transform)
	{
		shader->Bind();
		shader->SetMat4("u_ViewProjection", m_SceneData->ViewProjectionMatrix);
		shader->SetMat4("u_Transform", transform);

		RenderCommand::DrawTriangles(vertexBuffer);
	}
}
