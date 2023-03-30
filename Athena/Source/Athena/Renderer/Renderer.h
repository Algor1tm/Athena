#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Athena/Renderer/Camera.h"
#include "Athena/Renderer/RenderCommand.h"
#include "Athena/Renderer/Environment.h"

#include "Athena/Math/Matrix.h"


namespace Athena
{
	class ATHENA_API Animator;
	class ATHENA_API VertexBuffer;
	class ATHENA_API Material;
	class ATHENA_API Texture2D;
	class ATHENA_API TextureCube;

	enum ShaderEnum
	{
		PBR,
		SHADOW_MAP_GEN,
		SKYBOX,

		EQUIRECTANGULAR_TO_CUBEMAP,
		IRRADIANCE_MAP_CONVOLUTION,
		ENVIRONMENT_MIP_FILTER,

		DEBUG_NORMALS,
		DEBUG_WIREFRAME,
		DEBUG_SHOW_CASCADES
	};


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

	enum class Antialising
	{
		NONE = 0,
		MSAA_2X = 1,
		MSAA_4X = 2,
		MSAA_8X = 3,
	};

	enum class DebugView
	{
		NONE = 0,
		NORMALS = 1,
		WIREFRAME = 2,
		SHOW_CASCADES = 3
	};

	struct ShadowSettings
	{
		bool EnableShadows = true;
		bool SoftShadows = true;
		float LightSize = 0.5f;
		float MaxDistance = 200.f;
		float FadeOut = 15.f;
		float ExponentialSplitFactor = 0.91f;
		float NearPlaneOffset = -15.f;
		float FarPlaneOffset = 15.f;
	};

	class ATHENA_API Renderer
	{
	public:
		static void Init(RendererAPI::API graphicsAPI);
		static void Shutdown();

		static void OnWindowResized(uint32 width, uint32 height);

		static void BeginScene(const CameraInfo& cameraInfo, const Ref<Environment>& environment);
		static void EndScene();

		static void BeginFrame();
		static void EndFrame();

		static Ref<Framebuffer> GetMainFramebuffer();
		static void BlitToScreen();

		static Ref<Framebuffer> GetShadowMapFramebuffer();

		static void SubmitLight(const DirectionalLight& dirLight);
		static void SubmitLight(const PointLight& pointLight);

		static void Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform = Matrix4::Identity(), int32 entityID = -1);

		static void ReloadShaders();
		static void PreProcessEnvironmentMap(const Ref<Texture2D>& equirectangularHDRMap, Ref<TextureCube>& prefilteredMap, Ref<TextureCube>& irradianceMap);

		static Ref<Texture2D> GetWhiteTexture();

		static inline RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

		static const ShadowSettings& GetShadowSettings();
		static void SetShadowSettings(const ShadowSettings& settings);

		static Antialising GetAntialiasingMethod();
		static void SetAntialiasingMethod(Antialising method);

		static DebugView GetDebugView();
		static void SetDebugView(DebugView view);

		static void SetRenderQueueLimit(uint32 limit);

		struct Statistics
		{
			uint32 GeometryCount = 0;
			uint32 PointLightsCount = 0;
			uint32 DirectionalLightsCount = 0;
			uint32 DrawCalls = 0;
		};

		static const Statistics& GetStatistics();
		static void ResetStats();

	private:
		static void RenderGeometry(ShaderEnum shader, bool useMaterials);

		static void ShadowMapPass();
		static void GeometryPass();
		static void DebugViewPass();
		static void SkyboxPass();

		static void ComputeCascadeSplits();
		static void ComputeCascadeSpaceMatrices(const DirectionalLight& light);
	};
}
