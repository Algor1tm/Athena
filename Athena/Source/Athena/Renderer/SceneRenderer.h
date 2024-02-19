#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Camera.h"
#include "Athena/Renderer/GPUProfiler.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/RenderPass.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/RenderCommandBuffer.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Light.h"
#include "Athena/Renderer/DrawList.h"
#include "Athena/Renderer/Pipeline.h"
#include "Athena/Renderer/SceneRenderer2D.h"

#include "Athena/Math/Matrix.h"


namespace Athena
{
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
		WIREFRAME = 1,
		SHADOW_CASCADES = 2
	};

	struct LightEnvironmentSettings
	{
		float Exposure = 1;
		float Gamma = 2.2f;
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

	struct BloomSettings
	{
		bool EnableBloom = true;
		float Intensity = 1;
		float Threshold = 1.5;
		float Knee = 0.1;
		float DirtIntensity = 2;
		Ref<Texture2D> DirtTexture;
	};

	struct SceneRendererSettings
	{
		LightEnvironmentSettings LightEnvironmentSettings;
		ShadowSettings ShadowSettings;
		BloomSettings BloomSettings;
		DebugView DebugView = DebugView::NONE;
		Antialising AntialisingMethod = Antialising::MSAA_2X;
	};


	struct CameraData
	{
		Matrix4 View;
		Matrix4 Projection;
		Matrix4 ViewProjection;
		Matrix4 RotationView;
		Vector3 Position;
	};

	struct SceneData
	{
		float Exposure = 1.f;
		float Gamma = 2.2f;
		float EnvironmentIntensity = 1.f;
		float EnvironmentLOD = 0.f;
	};

	struct LightData
	{
		DirectionalLight DirectionalLights[ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT];
		uint32 DirectionalLightCount = 0;

		PointLight PointLights[ShaderDef::MAX_POINT_LIGHT_COUNT];
		uint32 PointLightCount = 0;
	};

	//struct ShadowsData
	//{
	//	struct CascadeSplitInfo
	//	{
	//		Vector2 LightFrustumPlanes;
	//		float SplitDepth;
	//		float __Padding;
	//	};

	//	Matrix4 LightViewProjMatrices[ShaderDef::SHADOW_CASCADES_COUNT];
	//	Matrix4 LightViewMatrices[ShaderDef::SHADOW_CASCADES_COUNT];
	//	CascadeSplitInfo CascadeSplits[ShaderDef::SHADOW_CASCADES_COUNT];
	//	float MaxDistance = 200.f;
	//	float FadeOut = 10.f;
	//	float LightSize = 0.5f;
	//	bool SoftShadows = true;
	//};

	struct ShadowsData
	{
		Matrix4 DirLightViewProjection[ShaderDef::SHADOW_CASCADES_COUNT];
	};

	struct BloomData
	{
		float Intensity;
		float Threshold;
		float Knee;
		float DirtIntensity;
		Vector2 TexelSize;
		bool EnableThreshold;
		int32 MipLevel;
	};

	struct SceneRendererStatistics
	{
		Time DirShadowMapPass;
		Time GeometryPass;
		Time SceneCompositePass;
		PipelineStatistics PipelineStats;
	};


	class ATHENA_API SceneRenderer : public RefCounted
	{
	public:
		static Ref<SceneRenderer> Create();
		~SceneRenderer();

		void Init();
		void Shutdown();

		SceneRendererSettings& GetSettings() { return m_Settings; }
		const SceneRendererStatistics& GetStatistics() { return m_Statistics; }
		Vector2u GetViewportSize() { return m_ViewportSize; }

		Ref<Image> GetFinalImage();

		void OnViewportResize(uint32 width, uint32 height);

		void BeginScene(const CameraInfo& cameraInfo);
		void EndScene();

		void Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Matrix4& transform = Matrix4::Identity());
		void Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform = Matrix4::Identity());
		void SubmitLightEnvironment(const LightEnvironment& lightEnv);

		Ref<RenderPass> GetRenderer2DPass() { return m_Renderer2DPass; }
		Ref<SceneRenderer2D> GetSceneRenderer2D() { return m_SceneRenderer2D; }

	private:
		void DirShadowMapPass();
		void GeometryPass();
		void SceneCompositePass();

	private:
		DrawList m_StaticGeometryList;
		DrawList m_AnimGeometryList;

		Ref<RenderPass> m_GeometryPass;
		Ref<Pipeline> m_StaticGeometryPipeline;
		Ref<Pipeline> m_AnimGeometryPipeline;
		Ref<Pipeline> m_SkyboxPipeline;

		Ref<RenderPass> m_DirShadowMapPass;
		Ref<Pipeline> m_DirShadowMapStaticPipeline;
		Ref<Pipeline> m_DirShadowMapAnimPipeline;

		Ref<RenderPass> m_CompositePass;
		Ref<Pipeline> m_CompositePipeline;

		Ref<RenderPass> m_Renderer2DPass;

		CameraData m_CameraData;
		SceneData m_SceneData;
		LightData m_LightData;
		ShadowsData m_ShadowsData;
		std::vector<Matrix4> m_BonesData;
		uint32 m_BonesDataOffset;

		Ref<UniformBuffer> m_CameraUBO;
		Ref<UniformBuffer> m_SceneUBO;
		Ref<StorageBuffer> m_LightSBO;
		Ref<UniformBuffer> m_ShadowsUBO;
		Ref<StorageBuffer> m_BonesSBO;

		Vector2u m_ViewportSize = { 1, 1 };

		Ref<SceneRenderer2D> m_SceneRenderer2D;
		Ref<RenderCommandBuffer> m_RenderCommandBuffer;
		Ref<GPUProfiler> m_Profiler;
		SceneRendererStatistics m_Statistics;
		SceneRendererSettings m_Settings;
	};
}
