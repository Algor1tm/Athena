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
		
		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BRDF_LUT;
		Ref<VertexBuffer> CubeVertexBuffer;
		Ref<VertexBuffer> QuadVertexBuffer;

		const uint32 BRDF_LUTResolution = 512;
	};

	RendererData s_Data;

	void Renderer::Init(Renderer::API api)
	{
		ATN_CORE_ASSERT(s_Data.RendererAPI == nullptr, "Renderer already exists!");

		switch (api)
		{
		case Renderer::API::OpenGL:
			s_Data.RendererAPI = CreateScope<GLRendererAPI>(); break;
		case Renderer::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		s_Data.API = api;
		s_Data.RendererAPI->Init();

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

		Ref<ComputeShader> ComputeBRDF_LUTShader = ComputeShader::Create("Assets/Shaders/BRDF_LUT");
		ComputeBRDF_LUTShader->Bind();

		s_Data.BRDF_LUT->BindAsImage();
		ComputeBRDF_LUTShader->Execute(width, height);

		ComputeBRDF_LUTShader->UnBind();

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

	void Renderer::OnWindowResized(uint32 width, uint32 height)
	{
		SceneRenderer::OnWindowResized(width, height);
	}

	void Renderer::SetViewport(uint32 x, uint32 y, uint32 width, uint32 height)
	{
		s_Data.RendererAPI->SetViewport(x, y, width, height);
	}

	void Renderer::Clear(const LinearColor& color)
	{
		s_Data.RendererAPI->Clear(color);
	}

	void Renderer::DrawTriangles(const Ref<VertexBuffer>& vertexBuffer, uint32 indexCount)
	{
		s_Data.RendererAPI->DrawTriangles(vertexBuffer, indexCount);
	}

	void Renderer::DrawLines(const Ref<VertexBuffer>& vertexBuffer, uint32 vertexCount)
	{
		s_Data.RendererAPI->DrawLines(vertexBuffer, vertexCount);
	}

	void Renderer::DisableCulling()
	{
		s_Data.RendererAPI->DisableCulling();
	}

	void Renderer::SetCullMode(CullFace face, CullDirection direction)
	{
		s_Data.RendererAPI->SetCullMode(face, direction);
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
