#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Athena/Renderer/Camera.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/Environment.h"

#include "Athena/Math/Matrix.h"


namespace Athena
{
	class ATHENA_API Animator;
	class ATHENA_API VertexBuffer;
	class ATHENA_API Material;
	class ATHENA_API Texture2D;
	class ATHENA_API TextureCube;

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

	class ATHENA_API SceneRenderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnWindowResized(uint32 width, uint32 height);

		static void BeginScene(const CameraInfo& cameraInfo, const Ref<Environment>& environment);
		static void EndScene();

		static void BeginEntityIDPass();
		static void FlushEntityIDs();
		static void EndEntityIDPass();

		static void BeginFrame();
		static void EndFrame();

		static Ref<Framebuffer> GetEntityIDFramebuffer();
		static Ref<Framebuffer> GetFinalFramebuffer();
		static void BlitToScreen();

		static void SubmitLight(const DirectionalLight& dirLight);
		static void SubmitLight(const PointLight& pointLight);

		static void Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform = Matrix4::Identity(), int32 entityID = -1);

		static void PreProcessEnvironmentMap(const Ref<Texture2D>& equirectangularHDRMap, Ref<TextureCube>& prefilteredMap, Ref<TextureCube>& irradianceMap);

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
		static void RenderGeometry(std::string_view shader, bool useMaterials);

		static void ShadowMapPass();
		static void GeometryPass();
		static void DebugViewPass();
		static void SkyboxPass();
		static void SceneCompositePass();

		static void ComputeCascadeSplits();
		static void ComputeCascadeSpaceMatrices(const DirectionalLight& light);
	};
}
