#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/RendererAPI.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Framebuffer.h"


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
		BONES_DATA = 8
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


	class ATHENA_API Renderer
	{
	public:
		enum API
		{
			None = 0,
			OpenGL = 1
		};

	public:
		static void Init(Renderer::API api);
		static void Shutdown();

		static Renderer::API GetAPI();

		static void OnWindowResized(uint32 width, uint32 height);

		static void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height);

		static void Clear(const LinearColor& color);

		static void DrawTriangles(const Ref<VertexBuffer>& vertexBuffer, uint32 indexCount = 0);
		static void DrawLines(const Ref<VertexBuffer>& vertexBuffer, uint32 vertexCount = 0);

		static void DisableCulling();
		static void SetCullMode(CullFace face = CullFace::BACK, CullDirection direction = CullDirection::COUNTER_CLOCKWISE);

		static Ref<Texture2D> GetBRDF_LUT();
		static Ref<Texture2D> GetWhiteTexture();

		static Ref<VertexBuffer> GetCubeVertexBuffer();
		static Ref<VertexBuffer> GetQuadVertexBuffer();

		static BufferLayout GetStaticVertexLayout();
		static BufferLayout GetAnimVertexLayout();
	};
}
