#include "Renderer.h"

#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/ConstantBuffer.h"
#include "Athena/Renderer/Material.h"

#include "Athena/Scene/SceneRenderer.h"


namespace Athena
{
	struct RendererData
	{
		Ref<Framebuffer> MainFramebuffer;
		BufferLayout VertexBufferLayout;
		Ref<Shader> PBRShader;

		struct SceneData
		{
			Matrix4 ModelViewProjection;
		};

		SceneData SceneDataBuffer;
		Ref<ConstantBuffer> SceneConstantBuffer;

		Ref<ConstantBuffer> MaterialConstantBuffer;
	};

	static RendererData s_Data;

	void Renderer::Init(RendererAPI::API graphicsAPI)
	{
		RendererAPI::Init(graphicsAPI);
		RenderCommand::Init();
		Renderer2D::Init();
		SceneRenderer::Init();


		FramebufferDescription fbDesc;
		fbDesc.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::DEPTH24STENCIL8 };
		fbDesc.Width = 1280;
		fbDesc.Height = 720;
		fbDesc.Samples = 4;

		s_Data.MainFramebuffer = Framebuffer::Create(fbDesc);

		s_Data.VertexBufferLayout =
		{
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float3, "a_Normal" }
		};

		s_Data.PBRShader = Shader::Create(s_Data.VertexBufferLayout, "Assets/Shaders/PBR");
		s_Data.SceneConstantBuffer = ConstantBuffer::Create(sizeof(RendererData::SceneData), 1);

		s_Data.MaterialConstantBuffer = ConstantBuffer::Create(sizeof(Material::ShaderData), 2);
	}

	void Renderer::Shutdown()
	{
		Renderer2D::Shutdown();
		SceneRenderer::Shutdown();
	}
	
	void Renderer::OnWindowResized(uint32 width, uint32 height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::BeginScene(const Matrix4& viewProjection)
	{
		s_Data.SceneDataBuffer.ModelViewProjection = viewProjection;
		s_Data.PBRShader->Bind();
	}
	
	void Renderer::EndScene()
	{

	}
	
	void Renderer::BeginFrame()
	{
		RenderCommand::BindFramebuffer(s_Data.MainFramebuffer);
	}

	void Renderer::EndFrame()
	{
		RenderCommand::UnBindFramebuffer();
	}

	void Renderer::RenderMesh(const Ref<StaticMesh>& mesh, const Ref<Material>& material, const Matrix4& transform)
	{
		Matrix4 viewProjection = s_Data.SceneDataBuffer.ModelViewProjection;
		s_Data.SceneDataBuffer.ModelViewProjection = transform * s_Data.SceneDataBuffer.ModelViewProjection;
		s_Data.SceneConstantBuffer->SetData(&s_Data.SceneDataBuffer, sizeof(RendererData::SceneData));
		
		if (material)
			s_Data.MaterialConstantBuffer->SetData(&material->Bind(), sizeof(Material::ShaderData));	
		
		if (mesh)
		{
			const auto& vertices = mesh->Vertices;
			for (uint32 i = 0; i < vertices.size(); ++i)
				RenderCommand::DrawTriangles(vertices[i]);
		}
		else
		{
			ATN_CORE_WARN("Renderer Warn: Attempt to render nullptr mesh!");
		}
		
		s_Data.SceneDataBuffer.ModelViewProjection = viewProjection;
	}

	void Renderer::Clear(const LinearColor& color)
	{
		RenderCommand::Clear(color);
	}

	Ref<Framebuffer> Renderer::GetFramebuffer()
	{
		return s_Data.MainFramebuffer;
	}

	const BufferLayout& Renderer::GetVertexBufferLayout()
	{
		return s_Data.VertexBufferLayout;
	}

	void Renderer::ReloadShaders()
	{
		Renderer2D::ReloadShaders();
		s_Data.PBRShader->Reload();
	}
}
