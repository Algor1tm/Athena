#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/CommandBuffer.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	class CommandBuffer;

	enum ShaderDef
	{
		MAX_DIRECTIONAL_LIGHT_COUNT = 32,
		MAX_POINT_LIGHT_COUNT = 32,

		SHADOW_CASCADES_COUNT = 4,

		MAX_NUM_BONES_PER_VERTEX = 4,
		MAX_NUM_BONES = 512,

		MAX_SKYBOX_MAP_LOD = 8,
	};

	enum TextureBinder
	{
		ALBEDO_MAP = 0,
		NORMAL_MAP = 1,
		ROUGHNESS_MAP = 2,
		METALNESS_MAP = 3,
		AMBIENT_OCCLUSION_MAP = 4,

		ENVIRONMENT_MAP = 5,
		IRRADIANCE_MAP = 6,
		BRDF_LUT = 7,

		SHADOW_MAP = 8,
		PCF_SAMPLER = 9
	};

	enum BufferBinder
	{
		RENDERER2D_CAMERA_DATA = 0,
		CAMERA_DATA = 1,
		SCENE_DATA = 2,
		ENVIRONMENT_MAP_DATA = 3,
		ENTITY_DATA = 4,
		MATERIAL_DATA = 5,
		SHADOWS_DATA = 6,
		LIGHT_DATA = 7,
		BONES_DATA = 8,
		BLOOM_DATA = 9
	};

	struct StaticVertex
	{
		Vector3 Position;
		Vector2 TexCoords;
		Vector3 Normal;
		Vector3 Tangent;
		Vector3 Bitangent;
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
	};

	class ATHENA_API ShaderLibrary;
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
		
		// TMP
		static Ref<CommandBuffer> GetCommandQueue();

		static Renderer::API GetAPI();

		static uint32 GetFramesInFlight();
		static uint32 GetCurrentFrameIndex();

		static void BeginFrame();
		static void EndFrame();

		static void Flush();
		static void SubmitResourceFree(std::function<void()>&& func);
		static void WaitDeviceIdle();
		
		static const FilePath& GetShaderPackDirectory();
		static const FilePath& GetShaderCacheDirectory();
		static Ref<ShaderLibrary> GetShaderLibrary();
		static const std::unordered_map<String, String>& GetGlobalShaderMacroses();
		static void SetGlobalShaderMacros(const String& name, const String& value);

		static Ref<Texture2D> GetBRDF_LUT();
		static Ref<Texture2D> GetWhiteTexture();
		static Ref<Texture2D> GetBlackTexture();

		static Ref<VertexBuffer> GetCubeVertexBuffer();
		static Ref<VertexBuffer> GetQuadVertexBuffer();

		static BufferLayout GetStaticVertexLayout();
		static BufferLayout GetAnimVertexLayout();

		struct Statistics
		{
			uint32 DrawCalls = 0;
			uint32 DispatchCalls = 0;
			uint32 ShadersBinded = 0;
			uint32 PipelinesBinded = 0;
			uint32 RenderPasses = 0;
			uint32 ComputePasses = 0;
		};

		static const Statistics& GetStatistics();
		static void ResetStats();
	};

	struct RendererConfig
	{
		Renderer::API API = Renderer::API::Vulkan;
		uint32 MaxFramesInFlight = 3;
	};
}
