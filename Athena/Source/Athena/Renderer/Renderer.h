#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/CommandQueue.h"
#include "Athena/Renderer/GPUBuffer.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/RenderCommandBuffer.h"
#include "Athena/Renderer/ComputePipeline.h"
#include "Athena/Renderer/Pipeline.h"


namespace Athena
{
	struct RenderCapabilities
	{
		String Name;
		uint64 VRAM;

		uint32 MaxImageDimension2D;
		uint32 MaxImageDimensionCube;
		uint32 MaxImageArrayLayers;

		float  MaxSamplerLodBias;
		float  MaxSamplerAnisotropy;

		uint32 MaxFramebufferWidth;
		uint32 MaxFramebufferHeight;
		uint32 MaxFramebufferLayers;
		uint32 MaxFramebufferColorAttachments;

		uint32 MaxUniformBufferRange;
		uint32 MaxStorageBufferRange;
		uint32 MaxPushConstantRange;

		uint32 MaxBoundDescriptorSets;
		uint32 MaxDescriptorSetSamplers;
		uint32 MaxDescriptorSetUnifromBuffers;
		uint32 MaxDescriptorSetStorageBuffers;
		uint32 MaxDescriptorSetSampledImages;
		uint32 MaxDescriptorSetStorageImages;
		uint32 MaxDescriptorSetInputAttachments;

		uint32 MaxViewportDimensions[2];
		uint32 MaxClipDistances;
		uint32 MaxCullDistances;
		float  LineWidthRange[2];

		uint32 MaxVertexInputAttributes;
		uint32 MaxVertexInputBindingStride;
		uint32 MaxFragmentInputComponents;
		uint32 MaxFragmentOutputAttachments;

		uint32 MaxComputeWorkGroupSize[3];
		uint32 MaxComputeSharedMemorySize;
		uint32 MaxComputeWorkGroupInvocations;

		bool TimestampComputeAndGraphics;
		float TimestampPeriod;
	};

	enum ShaderDef
	{
		MAX_DIRECTIONAL_LIGHT_COUNT = 8,
		MAX_POINT_LIGHT_COUNT = 512,
		MAX_SPOT_LIGHT_COUNT = 32,
		MAX_POINT_LIGHT_COUNT_PER_TILE = 31,
		LIGHT_TILE_SIZE = 16,

		SHADOW_CASCADES_COUNT = 4,

		MAX_NUM_BONES_PER_VERTEX = 4,
		MAX_SKYBOX_MAP_LOD = 8,
	};

	class ATHENA_API ShaderPack;
	struct RendererConfig;

	class ATHENA_API Renderer
	{
	public:
		enum API
		{
			None = 0,
			Vulkan = 1
		};

	public:
		static void Init(const RendererConfig& config);
		static void Shutdown();

		static const RendererConfig& GetConfig();
		static Renderer::API GetAPI();
		
		static uint32 GetFramesInFlight();
		static uint32 GetCurrentFrameIndex();

		template <typename FuncT>
		static void Defer(FuncT&& func)
		{
			GetResourceFreeQueue().Submit(std::forward<FuncT>(func));
		}

		template <typename FuncT>
		static void SubmitResourceFree(FuncT&& func)
		{
			GetResourceFreeQueue().Submit(std::forward<FuncT>(func));
		}

		static void BeginFrame();
		static void EndFrame();

		static void RenderGeometryInstanced(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material = nullptr, uint32 instanceCount = 1, uint32 firstInstance = 0);
		static void RenderGeometry(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material = nullptr, uint32 offset = 0, uint32 count = 0);
		static void RenderFullscreen(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<Material>& material = nullptr);
		static void BindInstanceRateBuffer(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<VertexBuffer> vertexBuffer);

		static void Dispatch(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<ComputePipeline>& pipeline, Vector3i imageSize, const Ref<Material>& material = nullptr);
		static void InsertMemoryBarrier(const Ref<RenderCommandBuffer>& cmdBuffer);
		static void InsertExecutionBarrier(const Ref<RenderCommandBuffer>& cmdBuffer);

		static void BlitMipMap(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Texture>& texture);
		static void BlitToScreen(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Texture2D>& texture);

		static void BeginDebugRegion(const Ref<RenderCommandBuffer>& cmdBuffer, std::string_view name, const Vector4& color);
		static void EndDebugRegion(const Ref<RenderCommandBuffer>& cmdBuffer);
		static void InsertDebugMarker(const Ref<RenderCommandBuffer>& cmdBuffer, std::string_view name, const Vector4& color);

		static Ref<RenderCommandBuffer> GetRenderCommandBuffer();
		static const RenderCapabilities& GetRenderCaps();
		static uint64 GetMemoryUsage();

		static const FilePath& GetShaderPackDirectory();
		static const FilePath& GetShaderCacheDirectory();
		static Ref<ShaderPack> GetShaderPack();
		static const std::unordered_map<String, String>& GetGlobalShaderMacroses();
		static void SetGlobalShaderMacros(const String& name, const String& value);

		static Ref<MaterialTable> GetMaterialTable();

	private:
		static CommandQueue& GetResourceFreeQueue();
	};

	struct RendererConfig
	{
		Renderer::API API = Renderer::API::Vulkan;
		uint32 MaxFramesInFlight = 3;
	};
}
