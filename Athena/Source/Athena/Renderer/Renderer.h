#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Framebuffer.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/RendererAPI.h"


namespace Athena
{
	enum ShaderConstants
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
		int BoneIDs[ShaderConstants::MAX_NUM_BONES_PER_VERTEX];
		float Weights[ShaderConstants::MAX_NUM_BONES_PER_VERTEX];
	};

	class ATHENA_API ShaderLibrary;
	struct RendererConfig;

	class ATHENA_API Renderer
	{
	public:
		enum API
		{
			None = 0,
			OpenGL = 1
		};

	public:
		static void Init(const RendererConfig& config);
		static void Shutdown();

		static Renderer::API GetAPI();
		
		static void BindShader(std::string_view name);
		static Ref<ShaderLibrary> GetShaderLibrary();
		static const String& GetGlobalShaderMacroses();

		static void OnWindowResized(uint32 width, uint32 height);

		static void BindPipeline(const Pipeline& pipeline);

		static void BeginRenderPass(const RenderPass& pass) ;
		static void EndRenderPass();

		static void BeginComputePass(const ComputePass& pass);
		static void EndComputePass();

		static void DrawTriangles(const Ref<VertexBuffer>& vertexBuffer, uint32 indexCount = 0);
		static void DrawLines(const Ref<VertexBuffer>& vertexBuffer, uint32 vertexCount = 0);

		static void Dispatch(uint32 x, uint32 y, uint32 z = 1, Vector3i workGroupSize = { 8, 4, 1 });

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
		Renderer::API API = Renderer::API::OpenGL;
	};
}
