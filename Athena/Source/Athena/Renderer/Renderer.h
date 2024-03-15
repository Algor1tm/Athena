#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/CommandQueue.h"
#include "Athena/Renderer/GPUBuffers.h"
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
		MAX_POINT_LIGHT_COUNT = 32,
		MAX_SPOT_LIGHT_COUNT = 8,

		SHADOW_CASCADES_COUNT = 4,

		MAX_NUM_BONES_PER_VERTEX = 4,
		MAX_NUM_BONES_PER_MESH = 120,
		MAX_NUM_ANIMATED_MESHES = 100,

		MAX_SKYBOX_MAP_LOD = 8,
	};

	struct StaticVertex
	{
		Vector3 Position;
		Vector2 TexCoords;
		Vector3 Normal;
		Vector3 Tangent;
		Vector3 Bitangent;

		static VertexLayout GetLayout()
		{
			return { 
				{ ShaderDataType::Float3, "a_Position"  }, 
				{ ShaderDataType::Float2, "a_TexCoords" }, 
				{ ShaderDataType::Float3, "a_Normal"    },
				{ ShaderDataType::Float3, "a_Tangent"   }, 
				{ ShaderDataType::Float3, "a_Bitangent" } };
		}
	};

	struct AnimVertex
	{
		Vector3 Position;
		Vector2 TexCoords;
		Vector3 Normal;
		Vector3 Tangent;
		Vector3 Bitangent;
		int BoneIDs[ShaderDef::MAX_NUM_BONES_PER_VERTEX];
		float Weights[ShaderDef::MAX_NUM_BONES_PER_VERTEX];

		static VertexLayout GetLayout()
		{
			return {
				{ ShaderDataType::Float3, "a_Position"  },
				{ ShaderDataType::Float2, "a_TexCoords" },
				{ ShaderDataType::Float3, "a_Normal"    },
				{ ShaderDataType::Float3, "a_Tangent"   },
				{ ShaderDataType::Float3, "a_Bitangent" },
				{ ShaderDataType::Int4,   "a_Tangent"   },
				{ ShaderDataType::Float4, "a_Bitangent" } };
		}
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

		static void RenderGeometry(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material = nullptr, uint32 vertexCount = 0);
		static void RenderFullscreenQuad(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<Material>& material = nullptr);
		static void RenderNDCCube(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<Material>& material = nullptr);

		static void Dispatch(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<ComputePipeline>& pipeline, Vector3i imageSize, const Ref<Material>& material = nullptr);
		static void MemoryDependency(const Ref<RenderCommandBuffer>& cmdBuffer);
		static void ExecutionDependency(const Ref<RenderCommandBuffer>& cmdBuffer);

		static void BlitToScreen(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Image>& image);

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

		static Ref<Texture2D> GetBRDF_LUT();
		static Ref<Texture2D> GetWhiteTexture();
		static Ref<Texture2D> GetBlackTexture();
		static Ref<TextureCube> GetBlackTextureCube();

		static Ref<VertexBuffer> GetCubeVertexBuffer();
		static Ref<VertexBuffer> GetQuadVertexBuffer();

	private:
		static CommandQueue& GetResourceFreeQueue();
	};

	struct RendererConfig
	{
		Renderer::API API = Renderer::API::Vulkan;
		uint32 MaxFramesInFlight = 3;
	};
}
