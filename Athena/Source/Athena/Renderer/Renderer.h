#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Athena/Renderer/RenderCommand.h"
#include "Athena/Renderer/Environment.h"

#include "Athena/Math/Matrix.h"


namespace Athena
{
	class ATHENA_API Animator;
	class ATHENA_API VertexBuffer;
	class ATHENA_API Material;
	class ATHENA_API Texture2D;
	class ATHENA_API Cubemap;

	enum ShaderLimits
	{
		MAX_DIRECTIONAL_LIGHT_COUNT = 32,
		MAX_POINT_LIGHT_COUNT = 32,

		MAX_NUM_BONES_PER_VERTEX = 4,
		MAX_NUM_BONES = 512,

		MAX_SKYBOX_MAP_LOD = 8
	};

	enum TextureBinder
	{
		ALBEDO_TEXTURE = 0,
		NORMAL_MAP = 1,
		ROUGHNESS_MAP = 2,
		METALNESS_MAP = 3,
		AMBIENT_OCCLUSION_MAP = 4,

		SKY_BOX = 5,
		IRRADIANCE_MAP = 6,
		BRDF_LUT = 7,
	};

	enum BufferBinder
	{
		RENDERER2D_CAMERA_DATA = 0,
		SCENE_DATA = 1,
		ENTITY_DATA = 2,
		MATERIAL_DATA = 3,
		LIGHT_DATA = 4,
		BONES_DATA = 5
	};

	enum class DebugView
	{
		NONE = 0,
		NORMALS = 1,
		WIREFRAME = 2
	};

	class ATHENA_API Renderer
	{
	public:

	public:
		static void Init(RendererAPI::API graphicsAPI);
		static void Shutdown();

		static void OnWindowResized(uint32 width, uint32 height);

		static void BeginScene(const Matrix4& viewMatrix, const Matrix4& projectionMatrix, const Ref<Environment>& environment);
		static void EndScene();

		static void BeginFrame();
		static void EndFrame();

		static void SubmitLight(const DirectionalLight& dirLight);
		static void SubmitLight(const PointLight& pointLight);

		static void Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Matrix4& transform = Matrix4::Identity(), int32 entityID = -1);
		static void SubmitWithAnimation(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform = Matrix4::Identity(), int32 entityID = -1);

		static void Clear(const LinearColor& color);
		static Ref<Framebuffer> GetMainFramebuffer();

		static void ReloadShaders();
		static void PreProcessEnvironmentMap(const Ref<Texture2D>& equirectangularHDRMap, Ref<Cubemap>& prefilteredMap, Ref<Cubemap>& irradianceMap);

		static inline RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

		static void SetRenderQueueLimit(uint32 limit);
		static void SetDebugView(DebugView view);

		struct Statistics
		{
			Time GeometryPass;
			Time SkyboxPass;

			uint32 GeometryCount = 0;
			uint32 PointLightsCount = 0;
			uint32 DirectionalLightsCount = 0;
			uint32 DrawCalls = 0;
		};

		static const Statistics& GetStatistics();
		static void ResetStats();

	private:
		static void RenderDebugView(DebugView view);

	};
}
