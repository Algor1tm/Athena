#include "Renderer.h"

#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/ConstantBuffer.h"
#include "Athena/Renderer/Material.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"

#include "Athena/Scene/SceneRenderer.h"


namespace Athena
{
	struct RendererData
	{
		Ref<Framebuffer> MainFramebuffer;
		BufferLayout VertexBufferLayout;

		Ref<Shader> PBRShader;
		Ref<Shader> SkyboxShader;
		Ref<Shader> EquirectangularToCubemapShader;
		Ref<Shader> GenIrradianceMapShader;

		Ref<VertexBuffer> CubeVertexBuffer;

		Ref<Environment> ActiveEnvironment;

		struct SceneData
		{
			Matrix4 ViewMatrix;
			Matrix4 ProjectionMatrix;
			Matrix4 TransformMatrix;
			Vector4 CameraPosition;
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
		fbDesc.Attachments = { TextureFormat::RGBA8, TextureFormat::RED_INTEGER, TextureFormat::DEPTH24STENCIL8 };
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

		BufferLayout cubeVBLayout = { { ShaderDataType::Float3, "a_Position" } };

		s_Data.SkyboxShader = Shader::Create(cubeVBLayout, "Assets/Shaders/Skybox");
		s_Data.EquirectangularToCubemapShader = Shader::Create(cubeVBLayout, "Assets/Shaders/EquirectangularToCubemap");
		s_Data.GenIrradianceMapShader = Shader::Create(cubeVBLayout, "Assets/Shaders/GenIrradianceMap");
		// Cube
		uint32 indices[] = { 1, 6, 2, 6, 1, 5,  0, 7, 4, 7, 0, 3,  4, 6, 5, 6, 4, 7,  0, 2, 3, 2, 0, 1,  0, 5, 1, 5, 0, 4,  3, 6, 7, 6, 3, 2 };
		Vector3 vertices[] = { {-1.f, -1.f, 1.f}, {1.f, -1.f, 1.f}, {1.f, -1.f, -1.f}, {-1.f, -1.f, -1.f}, {-1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, -1.f}, {-1.f, 1.f, -1.f} };

		VertexBufferDescription desc;
		desc.Data = vertices;
		desc.Size = sizeof(vertices);
		desc.pBufferLayout = &cubeVBLayout;
		desc.pIndexBuffer = IndexBuffer::Create(indices, std::size(indices));
		desc.Usage = BufferUsage::STATIC;

		s_Data.CubeVertexBuffer = VertexBuffer::Create(desc);

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

	void Renderer::BeginScene(const Matrix4& viewMatrix, const Matrix4& projectionMatrix, const Ref<Environment>& environment)
	{
		s_Data.SceneDataBuffer.ViewMatrix = viewMatrix;
		s_Data.SceneDataBuffer.ProjectionMatrix = projectionMatrix;
		s_Data.SceneDataBuffer.CameraPosition = Math::AffineInverse(viewMatrix)[3];
		s_Data.PBRShader->Bind();

		s_Data.ActiveEnvironment = environment;
		s_Data.LightConstantBuffer->SetData(&environment->DirLight, sizeof(DirectionalLight));
	}
	
	void Renderer::EndScene()
	{
		if (s_Data.ActiveEnvironment->Skybox)
		{
			// Remove translation
			s_Data.SceneDataBuffer.ViewMatrix[3][0] = 0;
			s_Data.SceneDataBuffer.ViewMatrix[3][1] = 0;
			s_Data.SceneDataBuffer.ViewMatrix[3][2] = 0;

			s_Data.SceneDataBuffer.TransformMatrix = Matrix4::Identity();

			s_Data.SkyboxShader->Bind();
			s_Data.ActiveEnvironment->Skybox->Bind();

			s_Data.SceneConstantBuffer->SetData(&s_Data.SceneDataBuffer, sizeof(RendererData::SceneData));

			RenderCommand::DrawTriangles(s_Data.CubeVertexBuffer);
		}

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
		
		s_Data.ActiveEnvironment->IrradianceMap->Bind(5);

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
		s_Data.SkyboxShader->Reload();
		s_Data.EquirectangularToCubemapShader->Reload();
		s_Data.GenIrradianceMapShader->Reload();
	}

	void Renderer::PreProcessEnvironmentMap(const Ref<Texture2D>& equirectangularHDRMap, const Ref<Environment>& envStorage)
	{
		uint32 width = equirectangularHDRMap->GetWidth();
		uint32 height = width;

		FramebufferDescription fbDesc;
		fbDesc.Attachments = { TextureFormat::RGB16F, TextureFormat::DEPTH32 };
		fbDesc.Width = width;
		fbDesc.Height = height;
		fbDesc.Samples = 1;

		Ref<Framebuffer> framebuffer = Framebuffer::Create(fbDesc);
		void* framebufferTexId = framebuffer->GetColorAttachmentRendererID(0);

		CubemapDescription cubeMapDesc;
		cubeMapDesc.Width = width;
		cubeMapDesc.Height = height;
		cubeMapDesc.Format = TextureFormat::RGB16F;

		Ref<Cubemap> envCubemap = Cubemap::Create(cubeMapDesc);
		
		Matrix4 captureProjection = Math::Perspective(Math::Radians(90.0f), 1.0f, 0.1f, 10.0f);
		Matrix4 captureViews[] =
		{
		   Math::LookAt(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f,  0.0f,  0.0f), Vector3(0.0f, -1.0f,  0.0f)),
		   Math::LookAt(Vector3(0.0f, 0.0f, 0.0f), Vector3(-1.0f, 0.0f,  0.0f), Vector3(0.0f, -1.0f,  0.0f)),
		   Math::LookAt(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f,  1.0f,  0.0f), Vector3(0.0f,  0.0f,  1.0f)),
		   Math::LookAt(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, -1.0f,  0.0f), Vector3(0.0f,  0.0f, -1.0f)),
		   Math::LookAt(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f,  0.0f,  1.0f), Vector3(0.0f, -1.0f,  0.0f)),
		   Math::LookAt(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f,  0.0f, -1.0f), Vector3(0.0f, -1.0f,  0.0f))
		};

		RenderCommand::BindFramebuffer(framebuffer);

		// Convert Equirectangular 2D texture to Cubemap
		s_Data.EquirectangularToCubemapShader->Bind();
		equirectangularHDRMap->Bind();

		s_Data.SceneDataBuffer.ProjectionMatrix = captureProjection;

		RenderCommand::SetViewport(0, 0, width, height);
		for (uint32 i = 0; i < 6; ++i)
		{
			s_Data.SceneDataBuffer.ViewMatrix = captureViews[i];
			s_Data.SceneConstantBuffer->SetData(&s_Data.SceneDataBuffer, sizeof(RendererData::SceneData));

			TextureTarget target = TextureTarget(static_cast<uint32>(TextureTarget::TEXTURE_CUBE_MAP_POSITIVE_X) + i);

			framebuffer->ReplaceAttachment(0, target, envCubemap->GetRendererID());
			RenderCommand::Clear({ 1, 1, 1, 1 });

			RenderCommand::DrawTriangles(s_Data.CubeVertexBuffer);
		}

		envStorage->Skybox = envCubemap;


		width = 32;
		height = 32;

		cubeMapDesc.Width = width;
		cubeMapDesc.Height = height;

		Ref<Cubemap> irradianceMap = Cubemap::Create(cubeMapDesc);

		framebuffer->ReplaceAttachment(0, TextureTarget::TEXTURE_2D, framebufferTexId);
		framebuffer->Resize(width, height);

		// Convolute Environment Map
		s_Data.GenIrradianceMapShader->Bind();
		envCubemap->Bind();

		RenderCommand::SetViewport(0, 0, width, height);
		for (uint32 i = 0; i < 6; ++i)
		{
			s_Data.SceneDataBuffer.ViewMatrix = captureViews[i];
			s_Data.SceneConstantBuffer->SetData(&s_Data.SceneDataBuffer, sizeof(RendererData::SceneData));

			TextureTarget target = TextureTarget(static_cast<uint32>(TextureTarget::TEXTURE_CUBE_MAP_POSITIVE_X) + i);

			framebuffer->ReplaceAttachment(0, target, irradianceMap->GetRendererID());
			RenderCommand::Clear({ 1, 1, 1, 1 });

			RenderCommand::DrawTriangles(s_Data.CubeVertexBuffer);
		}

		envStorage->IrradianceMap = irradianceMap;

		framebuffer->ReplaceAttachment(0, TextureTarget::TEXTURE_2D, framebufferTexId);
		RenderCommand::UnBindFramebuffer();
	}
}
