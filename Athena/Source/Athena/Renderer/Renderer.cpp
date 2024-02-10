#include "Renderer.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/FileSystem.h"

#include "Athena/Renderer/RendererAPI.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/ComputePipeline.h"
#include "Athena/Renderer/ComputePass.h"


namespace Athena
{
	struct RendererData
	{
		RendererConfig Config;
		Ref<RendererAPI> RendererAPI;
		uint32 CurrentFrameIndex = 0;
		uint32 CurrentResourceFreeQueueIndex = 0;
		RenderCapabilities RenderCaps;

		CommandQueue RenderThreadCommandQueue;
		std::vector<CommandQueue> ResourceFreeQueues; 
		Ref<RenderCommandBuffer> RenderCommandBuffer;

		FilePath ShaderPackDirectory;
		FilePath ShaderCacheDirectory;
		std::unordered_map<String, String> GlobalShaderMacroses;
		Ref<ShaderPack> ShaderPack;
		Ref<MaterialTable> MaterialTable;

		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BlackTexture;
		Ref<TextureCube> BlackTextureCube;
		Ref<Texture2D> BRDF_LUT;
		Ref<VertexBuffer> CubeVertexBuffer;
		Ref<VertexBuffer> QuadVertexBuffer;
	};

	RendererData s_Data;

	void Renderer::Init(const RendererConfig& config)
	{
		ATN_CORE_VERIFY(s_Data.RendererAPI == nullptr, "Renderer already exists!");

		s_Data.Config = config;
		s_Data.CurrentFrameIndex = config.MaxFramesInFlight - 1;
		s_Data.CurrentResourceFreeQueueIndex = s_Data.CurrentFrameIndex;

		s_Data.RenderThreadCommandQueue = CommandQueue(1024 * 1024 * 10);		// 10 Mb

		s_Data.ResourceFreeQueues.resize(s_Data.Config.MaxFramesInFlight + 1);
		for (uint32 i = 0; i < s_Data.ResourceFreeQueues.size(); ++i)
		{
			s_Data.ResourceFreeQueues[i] = CommandQueue(1024 * 1024 * 2);	// 2 Mb
		}

		const FilePath& resourcesPath = Application::Get().GetConfig().EngineResourcesPath;
		s_Data.ShaderCacheDirectory = resourcesPath / "Cache/ShaderPack";
		s_Data.ShaderPackDirectory = resourcesPath / "ShaderPack";

		if (!FileSystem::Exists(s_Data.ShaderCacheDirectory))
			FileSystem::CreateDirectory(s_Data.ShaderCacheDirectory);

		s_Data.RendererAPI = RendererAPI::Create(s_Data.Config.API);
		s_Data.RendererAPI->Init();
		s_Data.RendererAPI->GetRenderCapabilities(s_Data.RenderCaps);

		RenderCommandBufferCreateInfo cmdBufferInfo;
		cmdBufferInfo.Name = "Renderer_MainCommandBuffer";
		cmdBufferInfo.Usage = RenderCommandBufferUsage::PRESENT;

		s_Data.RenderCommandBuffer = RenderCommandBuffer::Create(cmdBufferInfo);

		Renderer::SetGlobalShaderMacros("MAX_DIRECTIONAL_LIGHT_COUNT", std::to_string(MAX_DIRECTIONAL_LIGHT_COUNT));
		Renderer::SetGlobalShaderMacros("MAX_POINT_LIGHT_COUNT", std::to_string(MAX_POINT_LIGHT_COUNT));
		Renderer::SetGlobalShaderMacros("MAX_SKYBOX_MAP_LOD", std::to_string(MAX_SKYBOX_MAP_LOD));

		s_Data.ShaderPack = ShaderPack::Create(s_Data.ShaderPackDirectory);
		s_Data.MaterialTable = Ref<MaterialTable>::Create();

		uint32 whiteTextureData = 0xffffffff;

		Texture2DCreateInfo texInfo;
		texInfo.Name = "Renderer_WhiteTexture";
		texInfo.Format = ImageFormat::RGBA8;
		texInfo.Usage = ImageUsage::DEFAULT;
		texInfo.InitialData = &whiteTextureData;
		texInfo.Width = 1;
		texInfo.Height = 1;
		texInfo.Layers = 1;
		texInfo.MipLevels = 1;
		texInfo.SamplerInfo.MinFilter = TextureFilter::NEAREST;
		texInfo.SamplerInfo.MagFilter = TextureFilter::NEAREST;
		texInfo.SamplerInfo.MipMapFilter = TextureFilter::NEAREST;
		texInfo.SamplerInfo.Wrap = TextureWrap::REPEAT;

		s_Data.WhiteTexture = Texture2D::Create(texInfo);

		uint32 blackTextureData = 0xff000000;
		texInfo.InitialData = &blackTextureData;
		texInfo.Name = "Renderer_BlackTexture";

		s_Data.BlackTexture = Texture2D::Create(texInfo); 

		TextureCubeCreateInfo texCubeInfo;
		texCubeInfo.Name = "Renderer_BlackTextureCube";
		texCubeInfo.Format = ImageFormat::RGBA8;
		texCubeInfo.Usage = ImageUsage::DEFAULT;
		texCubeInfo.InitialData = &blackTextureData;
		texCubeInfo.Width = 1;
		texCubeInfo.Height = 1;
		texCubeInfo.MipLevels = 1;
		texCubeInfo.SamplerInfo.MinFilter = TextureFilter::NEAREST;
		texCubeInfo.SamplerInfo.MagFilter = TextureFilter::NEAREST;
		texCubeInfo.SamplerInfo.MipMapFilter = TextureFilter::NEAREST;
		texCubeInfo.SamplerInfo.Wrap = TextureWrap::REPEAT;

		s_Data.BlackTextureCube = TextureCube::Create(texCubeInfo);

		Vector3 cubeVertices[] = { {-1.f, -1.f, 1.f}, {1.f, -1.f, 1.f}, {1.f, -1.f, -1.f}, {-1.f, -1.f, -1.f}, {-1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, -1.f}, {-1.f, 1.f, -1.f} };
		uint32 cubeIndices[] = { 1, 6, 2, 6, 1, 5,  0, 7, 4, 7, 0, 3,  4, 6, 5, 6, 4, 7,  0, 2, 3, 2, 0, 1,  0, 5, 1, 5, 0, 4,  3, 6, 7, 6, 3, 2 };

		IndexBufferCreateInfo indexBufInfo;
		indexBufInfo.Name = "Renderer_CubeIB";
		indexBufInfo.Data = cubeIndices;
		indexBufInfo.Count = std::size(cubeIndices);
		indexBufInfo.Usage = BufferUsage::STATIC;

		VertexBufferCreateInfo vertexBufInfo;
		vertexBufInfo.Name = "Renderer_CubeVB";
		vertexBufInfo.Data = cubeVertices;
		vertexBufInfo.Size = sizeof(cubeVertices);
		vertexBufInfo.IndexBuffer = IndexBuffer::Create(indexBufInfo);
		vertexBufInfo.Usage = BufferUsage::STATIC;

		s_Data.CubeVertexBuffer = VertexBuffer::Create(vertexBufInfo);

		uint32 quadIndices[] = { 0, 1, 2, 2, 3, 0 };
		float quadVertices[] = { -1.f,  1.f,  0.f, 0.f,
								  1.f,  1.f,  1.f, 0.f,
								  1.f, -1.f,  1.f, -1.f,
								 -1.f, -1.f,  0.f, -1.f, };

		indexBufInfo.Name = "Renderer_QuadIB";
		indexBufInfo.Data = quadIndices;
		indexBufInfo.Count = std::size(quadIndices);

		vertexBufInfo.Name = "Renderer_QuadVB";
		vertexBufInfo.Data = quadVertices;
		vertexBufInfo.Size = sizeof(quadVertices);
		vertexBufInfo.IndexBuffer = IndexBuffer::Create(indexBufInfo);

		s_Data.QuadVertexBuffer = VertexBuffer::Create(vertexBufInfo);

		// BRDF_LUT GENERATION
		{
			Texture2DCreateInfo brdfLutInfo;
			brdfLutInfo.Name = "Renderer_BRDF_LUT";
			brdfLutInfo.Format = ImageFormat::RG16F;
			brdfLutInfo.Usage = ImageUsage(ImageUsage::STORAGE | ImageUsage::SAMPLED);
			brdfLutInfo.InitialData = nullptr;
			brdfLutInfo.Width = 512;
			brdfLutInfo.Height = 512;
			brdfLutInfo.Layers = 1;
			brdfLutInfo.MipLevels = 1;
			brdfLutInfo.SamplerInfo.MinFilter = TextureFilter::LINEAR;
			brdfLutInfo.SamplerInfo.MagFilter = TextureFilter::LINEAR;
			brdfLutInfo.SamplerInfo.MipMapFilter = TextureFilter::LINEAR;
			brdfLutInfo.SamplerInfo.Wrap = TextureWrap::CLAMP_TO_EDGE;

			s_Data.BRDF_LUT = Texture2D::Create(brdfLutInfo);

			ComputePassCreateInfo passInfo;
			passInfo.Name = "BRDF_LUT_Pass";
			passInfo.Outputs.push_back(s_Data.BRDF_LUT->GetImage());

			Ref<ComputePass> pass = ComputePass::Create(passInfo);

			ComputePipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "BRDF_LUT_Pipeline";
			pipelineInfo.Shader = GetShaderPack()->Get("BRDF_LUT");
			pipelineInfo.WorkGroupSize = { 8, 4, 1 };
			
			Ref<ComputePipeline> pipeline = ComputePipeline::Create(pipelineInfo);
			pipeline->SetInput("u_BRDF_LUT", s_Data.BRDF_LUT);
			pipeline->Bake();
			
			cmdBufferInfo.Name = "BRDF_LUT_Generation";
			cmdBufferInfo.Usage = RenderCommandBufferUsage::IMMEDIATE;
			
			Ref<RenderCommandBuffer> commandBuffer = RenderCommandBuffer::Create(cmdBufferInfo);
			
			commandBuffer->Begin();
			{
				pass->Begin(commandBuffer);

				pipeline->Bind(commandBuffer);
				Renderer::Dispatch(commandBuffer, pipeline, { brdfLutInfo.Width, brdfLutInfo.Height, 1 });

				pass->End(commandBuffer);
			}
			commandBuffer->End();
			commandBuffer->Submit();
		}
	}

	void Renderer::Shutdown()
	{
		s_Data.MaterialTable.Release();

		s_Data.WhiteTexture.Release();
		s_Data.BlackTexture.Release();
		s_Data.BlackTextureCube.Release();
		s_Data.BRDF_LUT.Release();
		s_Data.CubeVertexBuffer.Release();
		s_Data.QuadVertexBuffer.Release();

		s_Data.ShaderPack.Release();

		s_Data.RendererAPI->Shutdown();
		s_Data.RendererAPI->WaitDeviceIdle();

		for (auto& queue : s_Data.ResourceFreeQueues)
		{
			queue.Flush();
		}
	}

	void Renderer::BeginFrame()
	{
		ATN_PROFILE_FUNC()
		s_Data.CurrentFrameIndex = (s_Data.CurrentFrameIndex + 1) % s_Data.Config.MaxFramesInFlight;
		s_Data.CurrentResourceFreeQueueIndex = (s_Data.CurrentResourceFreeQueueIndex + 1) % (s_Data.Config.MaxFramesInFlight + 1);

		Application::Get().GetWindow().GetSwapChain()->AcquireImage();

		// Free resources in queue
		// If resource is submitted to be freed on 'i' frame index, 
		// then it will be freed on 'i + FramesInFlight + 1' frame, so it guarantees
		// that currently used resource will not be freed
		{
			ATN_PROFILE_SCOPE("ResourceFreeQueue::Flush")
			s_Data.ResourceFreeQueues[s_Data.CurrentResourceFreeQueueIndex].Flush();
		}

		s_Data.RendererAPI->OnUpdate();
		s_Data.RenderCommandBuffer->Begin();
	}

	void Renderer::EndFrame()
	{
		ATN_PROFILE_FUNC()
		s_Data.RenderCommandBuffer->End();
		s_Data.RenderCommandBuffer->Submit();
	}

	void Renderer::WaitAndRender()
	{
		ATN_PROFILE_FUNC()
		Timer timer = Timer();

		s_Data.RenderThreadCommandQueue.Flush();

		Application::Get().GetStats().Renderer_WaitAndRender = timer.ElapsedTime();
	}

	void Renderer::RenderGeometry(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, uint32 vertexCount)
	{
		s_Data.RendererAPI->RenderGeometry(cmdBuffer, pipeline, vertexBuffer, material, vertexCount);
	}

	void Renderer::RenderFullscreenQuad(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<Material>& material)
	{
		s_Data.RendererAPI->RenderGeometry(cmdBuffer, pipeline, s_Data.QuadVertexBuffer, material);
	}

	void Renderer::RenderNDCCube(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<Material>& material)
	{
		s_Data.RendererAPI->RenderGeometry(cmdBuffer, pipeline, s_Data.CubeVertexBuffer, material);
	}

	void Renderer::Dispatch(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<ComputePipeline>& pipeline, Vector3i imageSize, const Ref<Material>& material)
	{
		s_Data.RendererAPI->Dispatch(cmdBuffer, pipeline, imageSize, material);
	}

	void Renderer::BlitToScreen(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Image>& image)
	{
		s_Data.RendererAPI->BlitToScreen(cmdBuffer, image);
	}

	void Renderer::BeginDebugRegion(const Ref<RenderCommandBuffer>& cmdBuffer, std::string_view name, const Vector4& color)
	{
		s_Data.RendererAPI->BeginDebugRegion(cmdBuffer, name, color);
	}

	void Renderer::EndDebugRegion(const Ref<RenderCommandBuffer>& cmdBuffer)
	{
		s_Data.RendererAPI->EndDebugRegion(cmdBuffer);
	}

	void Renderer::InsertDebugMarker(const Ref<RenderCommandBuffer>& cmdBuffer, std::string_view name, const Vector4& color)
	{
		s_Data.RendererAPI->InsertDebugMarker(cmdBuffer, name, color);
	}

	Ref<RenderCommandBuffer> Renderer::GetRenderCommandBuffer()
	{
		return s_Data.RenderCommandBuffer;
	}

	const RendererConfig& Renderer::GetConfig()
	{
		return s_Data.Config;
	}

	Renderer::API Renderer::GetAPI()
	{
		return s_Data.Config.API;
	}

	uint32 Renderer::GetFramesInFlight()
	{
		return s_Data.Config.MaxFramesInFlight;
	}

	uint32 Renderer::GetCurrentFrameIndex()
	{
		return s_Data.CurrentFrameIndex;
	}

	const FilePath& Renderer::GetShaderPackDirectory()
	{
		return s_Data.ShaderPackDirectory;
	}

	const FilePath& Renderer::GetShaderCacheDirectory()
	{
		return s_Data.ShaderCacheDirectory;
	}

	Ref<ShaderPack> Renderer::GetShaderPack()
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

	Ref<MaterialTable> Renderer::GetMaterialTable()
	{
		return s_Data.MaterialTable;
	}

	const RenderCapabilities& Renderer::GetRenderCaps()
	{
		return s_Data.RenderCaps;
	}

	uint64 Renderer::GetMemoryUsage()
	{
		return s_Data.RendererAPI->GetMemoryUsage();
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

	Ref<TextureCube> Renderer::GetBlackTextureCube()
	{
		return s_Data.BlackTextureCube;
	}

	Ref<VertexBuffer> Renderer::GetCubeVertexBuffer()
	{
		return s_Data.CubeVertexBuffer;
	}

	Ref<VertexBuffer> Renderer::GetQuadVertexBuffer()
	{
		return s_Data.QuadVertexBuffer;
	}

	CommandQueue& Renderer::GetRenderThreadCommandQueue()
	{
		return s_Data.RenderThreadCommandQueue;
	}

	CommandQueue& Renderer::GetResourceFreeQueue()
	{
		return s_Data.ResourceFreeQueues[s_Data.CurrentResourceFreeQueueIndex];
	}
}
