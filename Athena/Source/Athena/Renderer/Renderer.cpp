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
		Ref<Shader> SkyboxShader;

		Ref<Environment> ActiveEnvironment;

		struct SceneData
		{
			Matrix4 ViewProjectionMatrix;
			Vector4 CameraPosition;
			Matrix4 TransformMatrix;
			int32 EntityID = -1;
		};

		SceneData SceneDataBuffer;

		Ref<ConstantBuffer> SceneConstantBuffer;
		Ref<ConstantBuffer> MaterialConstantBuffer;
		Ref<ConstantBuffer> LightConstantBuffer;
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
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float3, "a_Tangent" },
			{ ShaderDataType::Float3, "a_Bitangent" }
		};

		s_Data.PBRShader = Shader::Create(s_Data.VertexBufferLayout, "Assets/Shaders/PBR");

		s_Data.SceneConstantBuffer = ConstantBuffer::Create(sizeof(RendererData::SceneData), 1);
		s_Data.MaterialConstantBuffer = ConstantBuffer::Create(sizeof(Material::ShaderData), 2);
		s_Data.LightConstantBuffer = ConstantBuffer::Create(sizeof(DirectionalLight), 3);
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

	void Renderer::BeginScene(const Matrix4& viewProjection, const Vector3& cameraPosition, const Ref<Environment>& environment)
	{
		s_Data.SceneDataBuffer.ViewProjectionMatrix = viewProjection;
		s_Data.SceneDataBuffer.CameraPosition = cameraPosition;
		s_Data.PBRShader->Bind();

		s_Data.ActiveEnvironment = environment;
		s_Data.LightConstantBuffer->SetData(&environment->DirLight, sizeof(DirectionalLight));
	}
	
	void Renderer::EndScene()
	{
		s_Data.ActiveEnvironment = nullptr;
	}
	
	void Renderer::BeginFrame()
	{
		RenderCommand::BindFramebuffer(s_Data.MainFramebuffer);
	}

	void Renderer::EndFrame()
	{
		RenderCommand::UnBindFramebuffer();
	}

	void Renderer::RenderMesh(const Ref<StaticMesh>& mesh, const Ref<Material>& material, const Matrix4& transform, int32 entityID)
	{
		s_Data.SceneDataBuffer.TransformMatrix = transform;
		s_Data.SceneDataBuffer.EntityID = entityID;
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
