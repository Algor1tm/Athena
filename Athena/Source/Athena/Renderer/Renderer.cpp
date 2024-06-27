#include "Renderer.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Renderer/Font.h"
#include "Athena/Renderer/RendererAPI.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/ComputePass.h"
#include "Athena/Renderer/TextureGenerator.h"


namespace Athena
{
	struct RendererData
	{
		RendererConfig Config;
		Ref<RendererAPI> RendererAPI;
		uint32 CurrentFrameIndex = 0;
		uint32 CurrentResourceFreeQueueIndex = 0;
		RenderCapabilities RenderCaps;

		std::vector<CommandQueue> ResourceFreeQueues; 
		Ref<RenderCommandBuffer> RenderCommandBuffer;

		FilePath ShaderPackDirectory;
		FilePath ShaderCacheDirectory;
		std::unordered_map<String, String> GlobalShaderMacroses;
		Ref<ShaderPack> ShaderPack;

		Ref<VertexBuffer> FullscreenVertexBuffer;
	};

	static RendererData s_Data;

	void Renderer::Init(const RendererConfig& config)
	{
		ATN_CORE_VERIFY(s_Data.RendererAPI == nullptr, "Renderer already exists!");

		s_Data.Config = config;
		s_Data.CurrentFrameIndex = config.MaxFramesInFlight - 1;
		s_Data.CurrentResourceFreeQueueIndex = s_Data.CurrentFrameIndex;

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
		Renderer::SetGlobalShaderMacros("MAX_SPOT_LIGHT_COUNT", std::to_string(MAX_SPOT_LIGHT_COUNT));
		Renderer::SetGlobalShaderMacros("MAX_POINT_LIGHT_COUNT_PER_TILE", std::to_string(MAX_POINT_LIGHT_COUNT_PER_TILE));
		Renderer::SetGlobalShaderMacros("LIGHT_TILE_SIZE", std::to_string(LIGHT_TILE_SIZE));
		Renderer::SetGlobalShaderMacros("MAX_SKYBOX_MAP_LOD", std::to_string(MAX_SKYBOX_MAP_LOD));
		Renderer::SetGlobalShaderMacros("MAX_NUM_BONES_PER_VERTEX", std::to_string(MAX_NUM_BONES_PER_VERTEX));
		Renderer::SetGlobalShaderMacros("SHADOW_CASCADES_COUNT", std::to_string(SHADOW_CASCADES_COUNT));
		Renderer::SetGlobalShaderMacros("HIZ_MIP_LEVEL_COUNT", std::to_string(HIZ_MIP_LEVEL_COUNT));
		Renderer::SetGlobalShaderMacros("PRECONVOLUTION_MIP_LEVEL_COUNT", std::to_string(PRECONVOLUTION_MIP_LEVEL_COUNT));
		Renderer::SetGlobalShaderMacros("DISPLAY_GAMMA", std::to_string(2.2));
		
		s_Data.ShaderPack = ShaderPack::Create(s_Data.ShaderPackDirectory);

		// Fake vertex buffer (does not actually used by vertex shader)
		// Contains fullscreen triangle positions
		// v0 = (-1, -1), v1 = (3, -1), v2 = (-1, 3).
		float fullscreenVertices[] = { -1, -1, 3, -1, -1, 3 };

		VertexBufferCreateInfo vertexBufInfo;
		vertexBufInfo.Name = "Renderer_FullscreenVB";
		vertexBufInfo.Data = fullscreenVertices;
		vertexBufInfo.Size = sizeof(fullscreenVertices);
		vertexBufInfo.IndexBuffer = nullptr;
		vertexBufInfo.Flags = BufferMemoryFlags::GPU_ONLY;

		s_Data.FullscreenVertexBuffer = VertexBuffer::Create(vertexBufInfo);

		TextureGenerator::Init();
		Font::Init();
	}

	void Renderer::Shutdown()
	{
		Font::Shutdown();
		TextureGenerator::Shutdown();

		s_Data.FullscreenVertexBuffer.Release();

		s_Data.ShaderPack.Release();

		s_Data.RendererAPI->WaitDeviceIdle();

		for (auto& queue : s_Data.ResourceFreeQueues)
		{
			queue.Flush();
		}

		s_Data.RendererAPI->Shutdown();
	}

	void Renderer::BeginFrame()
	{
		ATN_PROFILE_FUNC();
		s_Data.CurrentFrameIndex = (s_Data.CurrentFrameIndex + 1) % s_Data.Config.MaxFramesInFlight;
		s_Data.CurrentResourceFreeQueueIndex = (s_Data.CurrentResourceFreeQueueIndex + 1) % (s_Data.Config.MaxFramesInFlight + 1);

		Application::Get().GetWindow().GetSwapChain()->AcquireImage();

		// Free resources in queue
		// If resource is submitted to be freed on 'i' frame index, 
		// then it will be freed on 'i + FramesInFlight + 1' frame, so it guarantees
		// that currently used resource will not be freed
		{
			ATN_PROFILE_SCOPE("ResourceFreeQueue::Flush");
			s_Data.ResourceFreeQueues[s_Data.CurrentResourceFreeQueueIndex].Flush();
		}

		s_Data.RendererAPI->OnUpdate();
		s_Data.RenderCommandBuffer->Begin();
	}

	void Renderer::EndFrame()
	{
		ATN_PROFILE_FUNC();
		s_Data.RenderCommandBuffer->End();
		s_Data.RenderCommandBuffer->Submit();
	}

	void Renderer::RenderGeometryInstanced(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, uint32 instanceCount, uint32 firstInstance)
	{
		s_Data.RendererAPI->RenderGeometryInstanced(cmdBuffer, pipeline, vertexBuffer, material, instanceCount, firstInstance);
	}

	void Renderer::RenderGeometry(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, uint32 offset, uint32 count)
	{
		s_Data.RendererAPI->RenderGeometry(cmdBuffer, pipeline, vertexBuffer, material, offset, count);
	}

	void Renderer::FullscreenPass(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<RenderPass>& pass, const Ref<Pipeline>& pipeline, const Ref<Material>& material)
	{
		pass->Begin(cmdBuffer);
		{
			pipeline->Bind(cmdBuffer);
			s_Data.RendererAPI->RenderGeometry(cmdBuffer, pipeline, s_Data.FullscreenVertexBuffer, material);
		}
		pass->End(cmdBuffer);
	}

	void Renderer::BindInstanceRateBuffer(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<VertexBuffer> vertexBuffer)
	{
		s_Data.RendererAPI->BindInstanceRateBuffer(cmdBuffer, vertexBuffer);
	}

	void Renderer::Dispatch(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<ComputePipeline>& pipeline, Vector3i imageSize, const Ref<Material>& material)
	{
		s_Data.RendererAPI->Dispatch(cmdBuffer, pipeline, imageSize, material);
	}

	void Renderer::InsertMemoryBarrier(const Ref<RenderCommandBuffer>& cmdBuffer)
	{
		s_Data.RendererAPI->InsertMemoryBarrier(cmdBuffer);
	}

	void Renderer::InsertExecutionBarrier(const Ref<RenderCommandBuffer>& cmdBuffer)
	{
		s_Data.RendererAPI->InsertExecutionBarrier(cmdBuffer);
	}

	void Renderer::BlitMipMap(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Texture>& texture)
	{
		s_Data.RendererAPI->BlitMipMap(cmdBuffer, texture);
	}

	void Renderer::BlitToScreen(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Texture2D>& texture)
	{
		s_Data.RendererAPI->BlitToScreen(cmdBuffer, texture);
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

	const RenderCapabilities& Renderer::GetRenderCaps()
	{
		return s_Data.RenderCaps;
	}

	uint64 Renderer::GetMemoryUsage()
	{
		return s_Data.RendererAPI->GetMemoryUsage();
	}

	CommandQueue& Renderer::GetResourceFreeQueue()
	{
		return s_Data.ResourceFreeQueues[s_Data.CurrentResourceFreeQueueIndex];
	}
}
