#include "Renderer.h"

#include "Athena/Renderer/SceneRenderer.h"
#include "Athena/Renderer/SceneRenderer2D.h"
#include "Athena/Renderer/Shader.h"

#include "Athena/Platform/OpenGL/GLRendererAPI.h"


namespace Athena
{
	struct RendererData
	{
		Scope<RendererAPI> RendererAPI;
		Renderer::API API = Renderer::API::None;
		
		Ref<ShaderLibrary> ShaderPack;
		String GlobalShaderMacroses;

		Renderer::Statistics Stats;

		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BRDF_LUT;
		Ref<VertexBuffer> CubeVertexBuffer;
		Ref<VertexBuffer> QuadVertexBuffer;

		const uint32 BRDF_LUTResolution = 512;
	};

	RendererData s_Data;

	void Renderer::Init(const RendererConfig& config)
	{
		ATN_CORE_ASSERT(s_Data.RendererAPI == nullptr, "Renderer already exists!");

		switch (config.API)
		{
		case Renderer::API::OpenGL:
			s_Data.RendererAPI = CreateScope<GLRendererAPI>(); break;
		case Renderer::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		s_Data.API = config.API;
		s_Data.RendererAPI->Init();

#define DEFINE(name, value) std::format("\r\n#define {} {}", name, value)

		s_Data.GlobalShaderMacroses += DEFINE("ALBEDO_MAP_BINDER", (int32_t)TextureBinder::ALBEDO_MAP);
		s_Data.GlobalShaderMacroses += DEFINE("NORMAL_MAP_BINDER", (int32_t)TextureBinder::NORMAL_MAP);
		s_Data.GlobalShaderMacroses += DEFINE("ROUGHNESS_MAP_BINDER", (int32_t)TextureBinder::ROUGHNESS_MAP);
		s_Data.GlobalShaderMacroses += DEFINE("METALNESS_MAP_BINDER", (int32_t)TextureBinder::METALNESS_MAP);
		s_Data.GlobalShaderMacroses += DEFINE("AO_MAP_BINDER", (int32_t)TextureBinder::AMBIENT_OCCLUSION_MAP);
		s_Data.GlobalShaderMacroses += DEFINE("ENVIRONMENT_MAP_BINDER", (int32_t)TextureBinder::ENVIRONMENT_MAP);
		s_Data.GlobalShaderMacroses += DEFINE("IRRADIANCE_MAP_BINDER", (int32_t)TextureBinder::IRRADIANCE_MAP);
		s_Data.GlobalShaderMacroses += DEFINE("BRDF_LUT_BINDER", (int32_t)TextureBinder::BRDF_LUT);
		s_Data.GlobalShaderMacroses += DEFINE("SHADOW_MAP_BINDER", (int32_t)TextureBinder::SHADOW_MAP);
		s_Data.GlobalShaderMacroses += DEFINE("PCF_SAMPLER_BINDER", (int32_t)TextureBinder::PCF_SAMPLER);
		
		s_Data.GlobalShaderMacroses += DEFINE("MAX_DIRECTIONAL_LIGHT_COUNT", (int32_t)ShaderConstants::MAX_DIRECTIONAL_LIGHT_COUNT);
		s_Data.GlobalShaderMacroses += DEFINE("MAX_POINT_LIGHT_COUNT", (int32_t)ShaderConstants::MAX_POINT_LIGHT_COUNT);
		s_Data.GlobalShaderMacroses += DEFINE("SHADOW_CASCADES_COUNT", (int32_t)ShaderConstants::SHADOW_CASCADES_COUNT);
		s_Data.GlobalShaderMacroses += DEFINE("MAX_NUM_BONES_PER_VERTEX", (int32_t)ShaderConstants::MAX_NUM_BONES_PER_VERTEX);
		s_Data.GlobalShaderMacroses += DEFINE("MAX_NUM_BONES", (int32_t)ShaderConstants::MAX_NUM_BONES);
		s_Data.GlobalShaderMacroses += DEFINE("MAX_SKYBOX_MAP_LOD", (int32_t)ShaderConstants::MAX_SKYBOX_MAP_LOD);

		s_Data.GlobalShaderMacroses += DEFINE("RENDERER2D_CAMERA_BUFFER_BINDER", (int32_t)BufferBinder::RENDERER2D_CAMERA_DATA);
		s_Data.GlobalShaderMacroses += DEFINE("CAMERA_BUFFER_BINDER", (int32_t)BufferBinder::CAMERA_DATA);
		s_Data.GlobalShaderMacroses += DEFINE("SCENE_BUFFER_BINDER", (int32_t)BufferBinder::SCENE_DATA);
		s_Data.GlobalShaderMacroses += DEFINE("ENVMAP_BUFFER_BINDER", (int32_t)BufferBinder::ENVIRONMENT_MAP_DATA);
		s_Data.GlobalShaderMacroses += DEFINE("ENTITY_BUFFER_BINDER", (int32_t)BufferBinder::ENTITY_DATA);
		s_Data.GlobalShaderMacroses += DEFINE("MATERIAL_BUFFER_BINDER", (int32_t)BufferBinder::MATERIAL_DATA);
		s_Data.GlobalShaderMacroses += DEFINE("SHADOWS_BUFFER_BINDER", (int32_t)BufferBinder::SHADOWS_DATA);
		s_Data.GlobalShaderMacroses += DEFINE("LIGHT_BUFFER_BINDER", (int32_t)BufferBinder::LIGHT_DATA);
		s_Data.GlobalShaderMacroses += DEFINE("BONES_BUFFER_BINDER", (int32_t)BufferBinder::BONES_DATA);
		s_Data.GlobalShaderMacroses += DEFINE("BLOOM_BUFFER_BINDER", (int32_t)BufferBinder::BLOOM_DATA);

		s_Data.GlobalShaderMacroses += DEFINE("PI", 3.14159265358979323846);

		s_Data.GlobalShaderMacroses += "\r\n";
		
#undef DEFINE

		s_Data.ShaderPack = CreateRef<ShaderLibrary>();

		s_Data.ShaderPack->LoadIncludeShader("PoissonDisk", config.ShaderPack / "PoissonDisk");

		s_Data.ShaderPack->Load("PBR", config.ShaderPack / "PBR");
		s_Data.ShaderPack->Load("DirShadowMap", config.ShaderPack / "DirShadowMap");
		s_Data.ShaderPack->Load("Skybox", config.ShaderPack / "Skybox");
		s_Data.ShaderPack->Load("BloomDownsample", config.ShaderPack / "BloomDownsample");
		s_Data.ShaderPack->Load("BloomUpsample", config.ShaderPack / "BloomUpsample");
		s_Data.ShaderPack->Load("SceneComposite", config.ShaderPack / "SceneComposite");

		s_Data.ShaderPack->Load("EntityID", config.ShaderPack / "EntityID");

		s_Data.ShaderPack->Load("EquirectangularToCubemap", config.ShaderPack / "EquirectangularToCubemap");
		s_Data.ShaderPack->Load("IrradianceMapConvolution", config.ShaderPack / "IrradianceMapConvolution");
		s_Data.ShaderPack->Load("EnvironmentMipFilter", config.ShaderPack / "EnvironmentMipFilter");
		s_Data.ShaderPack->Load("BRDF_LUT", config.ShaderPack / "BRDF_LUT");

		s_Data.ShaderPack->Load("Debug_Wireframe", config.ShaderPack / "Debug/Wireframe");
		s_Data.ShaderPack->Load("Debug_ShadowCascades", config.ShaderPack / "Debug/ShadowCascades");

		s_Data.ShaderPack->Load("Renderer2D_Quad", config.ShaderPack / "Renderer2D/Quad");
		s_Data.ShaderPack->Load("Renderer2D_Circle", config.ShaderPack / "Renderer2D/Circle");
		s_Data.ShaderPack->Load("Renderer2D_Line", config.ShaderPack / "Renderer2D/Line");
		s_Data.ShaderPack->Load("Renderer2D_EntityID", config.ShaderPack / "Renderer2D/EntityID");


		uint32 cubeIndices[] = { 1, 6, 2, 6, 1, 5,  0, 7, 4, 7, 0, 3,  4, 6, 5, 6, 4, 7,  0, 2, 3, 2, 0, 1,  0, 5, 1, 5, 0, 4,  3, 6, 7, 6, 3, 2 };
		Vector3 cubeVertices[] = { {-1.f, -1.f, 1.f}, {1.f, -1.f, 1.f}, {1.f, -1.f, -1.f}, {-1.f, -1.f, -1.f}, {-1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, -1.f}, {-1.f, 1.f, -1.f} };

		VertexBufferDescription cubeVBdesc;
		cubeVBdesc.Data = cubeVertices;
		cubeVBdesc.Size = sizeof(cubeVertices);
		cubeVBdesc.Layout = { { ShaderDataType::Float3, "a_Position" } };
		cubeVBdesc.IndexBuffer = IndexBuffer::Create(cubeIndices, std::size(cubeIndices));
		cubeVBdesc.Usage = BufferUsage::STATIC;

		s_Data.CubeVertexBuffer = VertexBuffer::Create(cubeVBdesc);


		uint32 quadIndices[] = { 0, 1, 2, 2, 3, 0 };
		float quadVertices[] = { -1.f, -1.f,  0.f, 0.f,
								  1.f, -1.f,  1.f, 0.f,
								  1.f,  1.f,  1.f, 1.f,
								 -1.f,  1.f,  0.f, 1.f, };

		VertexBufferDescription quadVBDesc;
		quadVBDesc.Data = quadVertices;
		quadVBDesc.Size = sizeof(quadVertices);
		quadVBDesc.Layout = { { ShaderDataType::Float2, "a_Position" }, { ShaderDataType::Float2, "a_TexCoords" } };
		quadVBDesc.IndexBuffer = IndexBuffer::Create(quadIndices, std::size(quadIndices));
		quadVBDesc.Usage = BufferUsage::STATIC;

		s_Data.QuadVertexBuffer = VertexBuffer::Create(quadVBDesc);

		s_Data.WhiteTexture = Texture2D::Create(TextureFormat::RGBA8, 1, 1);
		uint32 whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32));

		uint32 width = s_Data.BRDF_LUTResolution;
		uint32 height = s_Data.BRDF_LUTResolution;


		TextureSamplerDescription sampler;
		sampler.MinFilter = TextureFilter::LINEAR;
		sampler.MagFilter = TextureFilter::LINEAR;
		sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

		s_Data.BRDF_LUT = Texture2D::Create(TextureFormat::RG16F, width, height, sampler);

		ComputePass computePass;
		computePass.Name = "BRDF_LUT";

		Renderer::BeginComputePass(computePass);
		{
			Ref<Shader> ComputeBRDF_LUTShader = GetShaderLibrary()->Get("BRDF_LUT");
			ComputeBRDF_LUTShader->Bind();

			s_Data.BRDF_LUT->BindAsImage();
			Renderer::Dispatch(width, height);

			ComputeBRDF_LUTShader->UnBind();
		}
		Renderer::EndComputePass();

		SceneRenderer::Init();
		SceneRenderer2D::Init();
	}

	void Renderer::Shutdown()
	{
		SceneRenderer2D::Shutdown();
		SceneRenderer::Shutdown();
	}

	Renderer::API Renderer::GetAPI()
	{
		return s_Data.API;
	}

	void Renderer::BindShader(std::string_view name)
	{
		GetShaderLibrary()->Get(name.data())->Bind();
		s_Data.Stats.ShadersBinded++;
	}

	Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return s_Data.ShaderPack;
	}

	const String& Renderer::GetGlobalShaderMacroses()
	{
		return s_Data.GlobalShaderMacroses;
	}

	void Renderer::OnWindowResized(uint32 width, uint32 height)
	{
		SceneRenderer::OnWindowResized(width, height);
	}

	void Renderer::BindPipeline(const Pipeline& pipeline)
	{
		s_Data.RendererAPI->BindPipeline(pipeline);
		s_Data.Stats.PipelinesBinded++;
	}

	void Renderer::BeginRenderPass(const RenderPass& pass)
	{
		s_Data.RendererAPI->BeginRenderPass(pass);
	}

	void Renderer::EndRenderPass()
	{
		s_Data.RendererAPI->EndRenderPass();
		s_Data.Stats.RenderPasses++;
	}

	void Renderer::BeginComputePass(const ComputePass& pass)
	{
		s_Data.RendererAPI->BeginComputePass(pass);
	}

	void Renderer::EndComputePass()
	{
		s_Data.RendererAPI->EndComputePass();
		s_Data.Stats.ComputePasses++;
	}

	void Renderer::DrawTriangles(const Ref<VertexBuffer>& vertexBuffer, uint32 indexCount)
	{
		s_Data.RendererAPI->DrawTriangles(vertexBuffer, indexCount);
		s_Data.Stats.DrawCalls++;
	}

	void Renderer::DrawLines(const Ref<VertexBuffer>& vertexBuffer, uint32 vertexCount)
	{
		s_Data.RendererAPI->DrawLines(vertexBuffer, vertexCount);
		s_Data.Stats.DrawCalls++;
	}

	void Renderer::Dispatch(uint32 x, uint32 y, uint32 z, Vector3i workGroupSize)
	{
		s_Data.RendererAPI->Dispatch(x, y, z, workGroupSize);
		s_Data.Stats.DispatchCalls++;
	}

	const Renderer::Statistics& Renderer::GetStatistics()
	{
		return s_Data.Stats;
	}

	void Renderer::ResetStats()
	{
		s_Data.Stats = Renderer::Statistics();
	}

	Ref<Texture2D> Renderer::GetBRDF_LUT()
	{
		return s_Data.BRDF_LUT;
	}

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_Data.WhiteTexture;
	}

	Ref<VertexBuffer> Renderer::GetCubeVertexBuffer()
	{
		return s_Data.CubeVertexBuffer;
	}

	Ref<VertexBuffer> Renderer::GetQuadVertexBuffer()
	{
		return s_Data.QuadVertexBuffer;
	}

	BufferLayout Renderer::GetStaticVertexLayout()
	{
		return 	
		{
			{ ShaderDataType::Float3, "a_Position"  },
			{ ShaderDataType::Float2, "a_TexCoord"  },
			{ ShaderDataType::Float3, "a_Normal"    },
			{ ShaderDataType::Float3, "a_Tangent"   },
			{ ShaderDataType::Float3, "a_Bitangent" }
		};
	}

	BufferLayout Renderer::GetAnimVertexLayout()
	{
		return 
		{
			{ ShaderDataType::Float3, "a_Position"  },
			{ ShaderDataType::Float2, "a_TexCoord"  },
			{ ShaderDataType::Float3, "a_Normal"    },
			{ ShaderDataType::Float3, "a_Tangent"   },
			{ ShaderDataType::Float3, "a_Bitangent" },
			{ ShaderDataType::Int4,	  "a_BoneIDs"   },
			{ ShaderDataType::Float4, "a_Weights"   },
		};
	}
}
