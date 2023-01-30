#include "Renderer.h"

#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/ConstantBuffer.h"
#include "Athena/Renderer/Material.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"

#include "Athena/Scene/SceneRenderer.h"

#include <deque>


namespace Athena
{
	struct DrawCallInfo
	{
		Ref<VertexBuffer> VertexBuffer;
		Ref<Material> Material;
		Matrix4 Transform;
		int32 EntityID;

		bool operator<(const DrawCallInfo& other) { return Material->GetName() < other.Material->GetName(); }
	};

	struct RendererData
	{
		std::deque<DrawCallInfo> RenderQueue;

		Ref<Framebuffer> MainFramebuffer;
		BufferLayout VertexBufferLayout;

		Ref<Shader> PBRShader;
		Ref<Shader> SkyboxShader;
		Ref<Shader> EquirectangularToCubemapShader;
		Ref<Shader> ComputeIrradianceMapShader;
		Ref<Shader> ComputePrefilterMapShader;

		Ref<VertexBuffer> CubeVertexBuffer;
		Ref<Texture2D> BRDF_LUT;

		Ref<Environment> ActiveEnvironment;

		struct SceneData
		{
			Matrix4 ViewMatrix;
			Matrix4 ProjectionMatrix;
			Matrix4 TransformMatrix;
			Vector4 CameraPosition;
			float SkyboxLOD = 0;
			float Exposure = 1;
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
		s_Data.ComputeIrradianceMapShader = Shader::Create(cubeVBLayout, "Assets/Shaders/ComputeIrradianceMap");
		s_Data.ComputePrefilterMapShader = Shader::Create(cubeVBLayout, "Assets/Shaders/ComputePrefilterMap");
		
		uint32 cubeIndices[] = { 1, 6, 2, 6, 1, 5,  0, 7, 4, 7, 0, 3,  4, 6, 5, 6, 4, 7,  0, 2, 3, 2, 0, 1,  0, 5, 1, 5, 0, 4,  3, 6, 7, 6, 3, 2 };
		Vector3 cubeVertices[] = { {-1.f, -1.f, 1.f}, {1.f, -1.f, 1.f}, {1.f, -1.f, -1.f}, {-1.f, -1.f, -1.f}, {-1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, -1.f}, {-1.f, 1.f, -1.f} };

		VertexBufferDescription cubeVBdesc;
		cubeVBdesc.Data = cubeVertices;
		cubeVBdesc.Size = sizeof(cubeVertices);
		cubeVBdesc.pBufferLayout = &cubeVBLayout;
		cubeVBdesc.pIndexBuffer = IndexBuffer::Create(cubeIndices, std::size(cubeIndices));
		cubeVBdesc.Usage = BufferUsage::STATIC;

		s_Data.CubeVertexBuffer = VertexBuffer::Create(cubeVBdesc);

		s_Data.SceneConstantBuffer = ConstantBuffer::Create(sizeof(RendererData::SceneData), ConstantBufferBinder::SCENE_DATA);
		s_Data.MaterialConstantBuffer = ConstantBuffer::Create(sizeof(Material::ShaderData), ConstantBufferBinder::MATERIAL_DATA);
		s_Data.LightConstantBuffer = ConstantBuffer::Create(sizeof(DirectionalLight), ConstantBufferBinder::LIGHT_DATA);

		// Create BRDF_LUT
		uint32 width = 512;
		uint32 height = 512;

		Texture2DDescription brdf_lutDesc;
		brdf_lutDesc.Width = width;
		brdf_lutDesc.Height = height;
		brdf_lutDesc.Format = TextureFormat::RG16F;
		brdf_lutDesc.Wrap = TextureWrap::CLAMP_TO_EDGE;

		s_Data.BRDF_LUT = Texture2D::Create(brdf_lutDesc);

		BufferLayout quadVBLayout = { { ShaderDataType::Float3, "a_Position" }, { ShaderDataType::Float2, "a_TexCoords" } };
		Ref<Shader> ComputeBRDF_LUTShader = Shader::Create(quadVBLayout, "Assets/Shaders/ComputeBRDF_LUT");

		uint32 quadIndices[] = { 0, 1, 2, 2, 3, 0 };
		float quadVertices[] = { -1.f, -1.f, 0.f,   0.f, 0.f,
								  1.f, -1.f, 0.f,	1.f, 0.f,
								  1.f,  1.f, 0.f,	1.f, 1.f,
								 -1.f,  1.f, 0.f,   0.f, 1.f };

		VertexBufferDescription desc;
		desc.Data = quadVertices;
		desc.Size = sizeof(quadVertices);
		desc.pBufferLayout = &quadVBLayout;
		desc.pIndexBuffer = IndexBuffer::Create(quadIndices, std::size(quadIndices));
		desc.Usage = BufferUsage::STATIC;

		Ref<VertexBuffer> quadVertexBuffer = VertexBuffer::Create(desc);

		fbDesc.Attachments = { TextureFormat::RG16F, TextureFormat::DEPTH32 };
		fbDesc.Width = width;
		fbDesc.Height = height;
		fbDesc.Samples = 1;
		Ref<Framebuffer> framebuffer = Framebuffer::Create(fbDesc);
		void *framebufferTexId = framebuffer->GetColorAttachmentRendererID(0);
		RenderCommand::BindFramebuffer(framebuffer);

		ComputeBRDF_LUTShader->Bind();

		RenderCommand::SetViewport(0, 0, width, height);

		framebuffer->ReplaceAttachment(0, TextureTarget::TEXTURE_2D, s_Data.BRDF_LUT->GetRendererID());
		RenderCommand::Clear({ 1, 1, 1, 1 });
		RenderCommand::DrawTriangles(quadVertexBuffer);

		framebuffer->ReplaceAttachment(0, TextureTarget::TEXTURE_2D, framebufferTexId);
		RenderCommand::UnBindFramebuffer();
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
		s_Data.ActiveEnvironment = environment;

		s_Data.SceneDataBuffer.ViewMatrix = viewMatrix;
		s_Data.SceneDataBuffer.ProjectionMatrix = projectionMatrix;
		s_Data.SceneDataBuffer.CameraPosition = Math::AffineInverse(viewMatrix)[3];
		s_Data.SceneDataBuffer.Exposure = s_Data.ActiveEnvironment->Exposure;

		s_Data.LightConstantBuffer->SetData(&environment->DirLight, sizeof(DirectionalLight));

		s_Data.PBRShader->Bind();
		if (s_Data.ActiveEnvironment->Skybox)
		{
			s_Data.ActiveEnvironment->Skybox->Bind();
		}

		s_Data.BRDF_LUT->Bind(TextureBinder::BRDF_LUT);
	}
	
	void Renderer::EndScene()
	{
		if (s_Data.ActiveEnvironment->Skybox)
		{
			s_Data.SceneDataBuffer.SkyboxLOD = s_Data.ActiveEnvironment->SkyboxLOD;

			// Remove translation
			s_Data.SceneDataBuffer.ViewMatrix[3][0] = 0;
			s_Data.SceneDataBuffer.ViewMatrix[3][1] = 0;
			s_Data.SceneDataBuffer.ViewMatrix[3][2] = 0;

			s_Data.SceneDataBuffer.TransformMatrix = Matrix4::Identity();

			s_Data.SkyboxShader->Bind();

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

	void Renderer::Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Matrix4& transform, int32 entityID)
	{
		if (vertexBuffer)
		{
			DrawCallInfo info;
			info.VertexBuffer = vertexBuffer;
			info.Material = material;
			info.Transform = transform;
			info.EntityID = entityID;

			s_Data.RenderQueue.push_back(info);
		}
		else
		{
			ATN_CORE_WARN("Renderer::Submit(): Attempt to submit nullptr vertexBuffer!");
		}
	}

	void Renderer::WaitAndRender()
	{
		std::sort(s_Data.RenderQueue.begin(), s_Data.RenderQueue.end());

		Ref<Material> lastMaterial = nullptr;
		for (const auto& info : s_Data.RenderQueue)
		{
			s_Data.SceneDataBuffer.TransformMatrix = info.Transform;
			s_Data.SceneDataBuffer.EntityID = info.EntityID;
			s_Data.SceneConstantBuffer->SetData(&s_Data.SceneDataBuffer, sizeof(RendererData::SceneData));

			if (lastMaterial == nullptr || *lastMaterial != *info.Material)
			{
				s_Data.MaterialConstantBuffer->SetData(&info.Material->Bind(), sizeof(Material::ShaderData));
				lastMaterial = info.Material;
			}

			RenderCommand::DrawTriangles(info.VertexBuffer);
		}

		s_Data.RenderQueue.clear();
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
		s_Data.ComputeIrradianceMapShader->Reload();
	}

	void Renderer::PreProcessEnvironmentMap(const Ref<Texture2D>& equirectangularHDRMap, Ref<Cubemap>& prefilteredMap, Ref<Cubemap>& irradianceMap)
	{
		uint32 width = 2048;
		uint32 height = 2048;

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
		cubeMapDesc.MinFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;

		Ref<Cubemap> skybox = Cubemap::Create(cubeMapDesc);
		
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

			framebuffer->ReplaceAttachment(0, target, skybox->GetRendererID());
			RenderCommand::Clear({ 1, 1, 1, 1 });

			RenderCommand::DrawTriangles(s_Data.CubeVertexBuffer);
		}
		skybox->GenerateMipMap(10);

		// Create Irradiance map
		width = 32;
		height = 32;

		cubeMapDesc.Width = width;
		cubeMapDesc.Height = height;
		cubeMapDesc.Format = TextureFormat::RGB16F;
		cubeMapDesc.MinFilter = TextureFilter::LINEAR;

		irradianceMap = Cubemap::Create(cubeMapDesc);

		framebuffer->ReplaceAttachment(0, TextureTarget::TEXTURE_2D, framebufferTexId);
		framebuffer->Resize(width, height);
		framebufferTexId = framebuffer->GetColorAttachmentRendererID(0);

		s_Data.ComputeIrradianceMapShader->Bind();
		skybox->Bind();

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

		// Create prefilter map
		width = 2048;
		height = 2048;
		
		cubeMapDesc.Width = width;
		cubeMapDesc.Height = height;
		cubeMapDesc.Format = TextureFormat::RGB16F;
		cubeMapDesc.MinFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
		prefilteredMap = Cubemap::Create(cubeMapDesc);

		const uint32 maxMipLevels = 11;

		prefilteredMap->GenerateMipMap(maxMipLevels);

		s_Data.ComputePrefilterMapShader->Bind();
		skybox->Bind();

		for (uint32 mip = 0; mip < maxMipLevels; ++mip)
		{
			uint32 mipWidth  = width * Math::Pow(0.5f, (float)mip);
			uint32 mipHeight = height * Math::Pow(0.5f, (float)mip);

			framebuffer->ReplaceAttachment(0, TextureTarget::TEXTURE_2D, framebufferTexId);
			framebuffer->Resize(mipWidth, mipHeight);
			framebufferTexId = framebuffer->GetColorAttachmentRendererID(0);

			RenderCommand::SetViewport(0, 0, mipWidth, mipHeight);

			float roughness = (float)mip / (float)(maxMipLevels - 1);
			s_Data.SceneDataBuffer.SkyboxLOD = roughness;
			for (uint32 i = 0; i < 6; ++i)
			{
				s_Data.SceneDataBuffer.ViewMatrix = captureViews[i];
				s_Data.SceneConstantBuffer->SetData(&s_Data.SceneDataBuffer, sizeof(RendererData::SceneData));

				TextureTarget target = TextureTarget(static_cast<uint32>(TextureTarget::TEXTURE_CUBE_MAP_POSITIVE_X) + i);
				framebuffer->ReplaceAttachment(0, target, prefilteredMap->GetRendererID(), mip);
				RenderCommand::Clear({ 1, 1, 1, 1 });

				RenderCommand::DrawTriangles(s_Data.CubeVertexBuffer);
			}
		}

		framebuffer->ReplaceAttachment(0, TextureTarget::TEXTURE_2D, framebufferTexId);
		RenderCommand::UnBindFramebuffer();
	}
}
