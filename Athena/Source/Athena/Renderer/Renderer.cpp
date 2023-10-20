#include "Renderer.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/FileSystem.h"

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
		Ref<CommandBuffer> RenderCommandBuffer;

		// Per frame in flight
		std::vector<std::vector<std::function<void()>>> ResourceFreeQueue;

		FilePath ShaderPackDirectory;
		FilePath ShaderCacheDirectory;
		std::unordered_map<String, String> GlobalShaderMacroses;
		Ref<ShaderLibrary> ShaderPack;

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
		s_Data.CurrentFrameIndex = config.MaxFramesInFlight - 1;

		s_Data.ResourceFreeQueue.resize(config.MaxFramesInFlight);

		const FilePath& resourcesPath = Application::Get().GetEngineResourcesPath();
		s_Data.ShaderPackDirectory = resourcesPath / "ShaderPack";
		s_Data.ShaderCacheDirectory = resourcesPath / "Cache/Shaders";

		if (!FileSystem::Exists(s_Data.ShaderCacheDirectory))
			FileSystem::CreateDirectory(s_Data.ShaderCacheDirectory);

		s_Data.RendererAPI = RendererAPI::Create(s_Data.API);
		s_Data.RendererAPI->Init();
		
		s_Data.RenderCommandBuffer = CommandBuffer::Create(CommandBufferUsage::PRESENT);

		uint32 whiteTextureData = 0xffffffff;

		//TextureCreateInfo texInfo;
		//texInfo.Data = &whiteTextureData;
		//texInfo.Width = 1;
		//texInfo.Height = 1;
		//texInfo.Layers = 1;
		//texInfo.MipLevels = 1;
		//texInfo.GenerateMipMap = false;
		//texInfo.sRGB = false;
		//texInfo.Format = TextureFormat::RGBA8;
		//texInfo.Usage = TextureUsage::SHADER_READ_ONLY;
		//texInfo.GenerateSampler = false;
		//texInfo.SamplerInfo.MinFilter = TextureFilter::NEAREST;
		//texInfo.SamplerInfo.MagFilter = TextureFilter::NEAREST;
		//texInfo.SamplerInfo.MipMapFilter = TextureFilter::NEAREST;
		//texInfo.SamplerInfo.Wrap = TextureWrap::REPEAT;

		//s_Data.WhiteTexture = Texture2D::Create(texInfo);

		//uint32 blackTextureData = 0xff000000;
		//texInfo.Data = &blackTextureData;

		//s_Data.BlackTexture = Texture2D::Create(texInfo);
	}

	void Renderer::Shutdown()
	{
		s_Data.RenderCommandBuffer.Reset();

		{
			for (const auto& queue : s_Data.ResourceFreeQueue)
			{
				for (const auto& func : queue)
					func();
			}

			s_Data.ResourceFreeQueue[s_Data.CurrentFrameIndex].clear();
		}

		s_Data.RendererAPI->Shutdown();
	}

	void Renderer::BeginFrame()
	{
		s_Data.CurrentFrameIndex = (s_Data.CurrentFrameIndex + 1) % s_Data.MaxFramesInFlight;

		// Free resources in queue
		{
			for (auto& func : s_Data.ResourceFreeQueue[s_Data.CurrentFrameIndex])
				func();
			s_Data.ResourceFreeQueue[s_Data.CurrentFrameIndex].clear();
		}

		// Acquire image from swapchain
		Application::Get().GetWindow().GetSwapChain()->AcquireImage();

		s_Data.RenderCommandBuffer->Begin();
	}

	void Renderer::EndFrame()
	{
		s_Data.RenderCommandBuffer->End();
		s_Data.RenderCommandBuffer->Flush();
	}

	Renderer::API Renderer::GetAPI()
	{
		return s_Data.API;
	}

	uint32 Renderer::GetFramesInFlight()
	{
		return s_Data.MaxFramesInFlight;
	}

	uint32 Renderer::GetCurrentFrameIndex()
	{
		return s_Data.CurrentFrameIndex;
	}

	void Renderer::SubmitResourceFree(std::function<void()>&& func)
	{
		s_Data.ResourceFreeQueue[Renderer::GetCurrentFrameIndex()].push_back(func);
	}

	void Renderer::WaitDeviceIdle()
	{
		s_Data.RendererAPI->WaitDeviceIdle();
	}

	const FilePath& Renderer::GetShaderPackDirectory()
	{
		return s_Data.ShaderPackDirectory;
	}

	const FilePath& Renderer::GetShaderCacheDirectory()
	{
		return s_Data.ShaderCacheDirectory;
	}

	Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return s_Data.ShaderPack;
	}

	const std::unordered_map<String, String>& Renderer::GetGlobalShaderMacroses()
	{
		return s_Data.GlobalShaderMacroses;
	}

	void Renderer::SetGlobalShaderMacros(const String& name, const String& value)
	{
		s_Data.GlobalShaderMacroses[name] = value;
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
