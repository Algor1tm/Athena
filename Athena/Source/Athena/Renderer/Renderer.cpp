#include "Renderer.h"

#include "Athena/Core/Application.h"

#include "Athena/Renderer/CommandBuffer.h"
#include "Athena/Renderer/RendererAPI.h"
#include "Athena/Renderer/Shader.h"


namespace Athena
{
	struct RendererData
	{
		Renderer::API API = Renderer::API::None;
		uint32 MaxFramesInFlight = 3;
		uint32 CurrentFrameIndex = 0;

		Ref<RendererAPI> RendererAPI;
		Ref<CommandBuffer> CommandQueue;
		
		Ref<ShaderLibrary> ShaderPack;
		String GlobalShaderMacroses;

		Renderer::Statistics Stats;

		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BlackTexture;
		Ref<Texture2D> BRDF_LUT;
		Ref<VertexBuffer> CubeVertexBuffer;
		Ref<VertexBuffer> QuadVertexBuffer;

		const uint32 BRDF_LUTResolution = 512;
	};

	RendererData s_Data;

	void Renderer::Init(const RendererConfig& config)
	{
		ATN_CORE_VERIFY(s_Data.RendererAPI == nullptr, "Renderer already exists!");

		s_Data.API = config.API;
		s_Data.MaxFramesInFlight = config.MaxFramesInFlight;

		s_Data.RendererAPI = RendererAPI::Create(s_Data.API);
		s_Data.RendererAPI->Init();
		                                                
		s_Data.CommandQueue = CommandBuffer::Create();
	}

	void Renderer::Shutdown()
	{
		s_Data.CommandQueue.Reset();
		s_Data.RendererAPI->Shutdown();
	}

	Ref<CommandBuffer> Renderer::GetCommandQueue()
	{
		return s_Data.CommandQueue;
	}

	void Renderer::BeginFrame()
	{
		Ref<SwapChain> swapChain = Application::Get().GetWindow().GetSwapChain();

		swapChain->AcquireImage();
		s_Data.CommandQueue->Begin();
	}

	void Renderer::EndFrame()
	{
		s_Data.CommandQueue->End();
		s_Data.RendererAPI->Submit(s_Data.CommandQueue);

		s_Data.CurrentFrameIndex = (s_Data.CurrentFrameIndex + 1) % s_Data.MaxFramesInFlight;
	}

	Renderer::API Renderer::GetAPI()
	{
		return s_Data.API;
	}

	uint32 Renderer::FramesInFlight()
	{
		return s_Data.MaxFramesInFlight;
	}

	uint32 Renderer::CurrentFrameIndex()
	{
		return s_Data.CurrentFrameIndex;
	}

	void Renderer::WaitDeviceIdle()
	{
		s_Data.RendererAPI->WaitDeviceIdle();
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

	Ref<Texture2D> Renderer::GetBlackTexture()
	{
		return s_Data.BlackTexture;
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
