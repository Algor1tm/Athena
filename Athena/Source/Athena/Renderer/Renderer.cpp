#include "Renderer.h"

#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/ConstantBuffer.h"


namespace Athena
{
	struct RendererData
	{
		Ref<Framebuffer> MainFramebuffer;

		struct SceneData
		{
			Matrix4 ModelViewProjection;
		};

		SceneData SceneDataBuffer;
		Ref<ConstantBuffer> SceneConstantBuffer;
	};

	static RendererData s_Data;

	void Renderer::Init(RendererAPI::API graphicsAPI)
	{
		RendererAPI::Init(graphicsAPI);
		RenderCommand::Init();
		Renderer2D::Init();


		FramebufferDescription fbDesc;
		fbDesc.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::DEPTH24STENCIL8 };
		fbDesc.Width = 1280;
		fbDesc.Height = 720;
		fbDesc.Samples = 1;
		s_Data.MainFramebuffer = Framebuffer::Create(fbDesc);

		s_Data.SceneConstantBuffer = ConstantBuffer::Create(sizeof(RendererData::SceneData), 1);
	}

	void Renderer::Shutdown()
	{
		Renderer2D::Shutdown();
	}
	
	void Renderer::OnWindowResized(uint32 width, uint32 height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::BeginScene(const Matrix4& viewProjection)
	{
		s_Data.SceneDataBuffer.ModelViewProjection = viewProjection;
	}
	
	void Renderer::EndScene()
	{

	}
	
	void Renderer::Submit(const Ref<Shader>& shader,
		const Ref<VertexBuffer>& vertexBuffer,
		const Matrix4& transform)
	{
		s_Data.SceneDataBuffer.ModelViewProjection = transform * s_Data.SceneDataBuffer.ModelViewProjection;
		s_Data.SceneConstantBuffer->SetData(&s_Data.SceneDataBuffer, sizeof(RendererData::SceneData));

		shader->Bind();

		RenderCommand::DrawTriangles(vertexBuffer);
	}

	void Renderer::Clear(const LinearColor& color)
	{
		RenderCommand::BindFramebuffer(s_Data.MainFramebuffer);
		RenderCommand::Clear(color);
	}

	Ref<Framebuffer> Renderer::GetFramebuffer()
	{
		return s_Data.MainFramebuffer;
	}
}
