#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Camera.h"
#include "Athena/Renderer/GPUProfiler.h"
#include "Athena/Renderer/GPUBuffer.h"
#include "Athena/Renderer/RenderPass.h"
#include "Athena/Renderer/ComputePass.h"
#include "Athena/Renderer/ComputePipeline.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/RenderCommandBuffer.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Light.h"
#include "Athena/Renderer/DrawList.h"
#include "Athena/Renderer/Pipeline.h"
#include "Athena/Renderer/Mesh.h"
#include "Athena/Renderer/SceneRenderer2D.h"

#include "Athena/Math/Matrix.h"


namespace Athena
{
	enum class Antialising
	{
		NONE = 0,
		FXAA
	};

	enum class DebugView
	{
		NONE = 0,
		SHADOW_CASCADES,
		LIGHT_COMPLEXITY,
		GBUFFER,
	};

	enum class TonemapMode
	{
		NONE = 0,
		ACES_FILMIC,
		ACES_TRUE,
	};

	struct PostProcessingSettings
	{
		TonemapMode TonemapMode = TonemapMode::ACES_FILMIC;
		float Exposure = 1.6f;
		Antialising AntialisingMethod = Antialising::NONE;
	};

	struct ShadowSettings
	{
		bool SoftShadows = true;
		float MaxDistance = 200.f;
		float FadeOut = 15.f;
		float CascadeBlendDistance = 0.5f;
		float CascadeSplit = 0.91f;
		float NearPlaneOffset = -70.f;
		float FarPlaneOffset = 15.f;
	};

	struct BloomSettings
	{
		bool Enable = true;
		float Intensity = 0.8f;
		float Threshold = 1.5f;
		float Knee = 0.1f;
		float DirtIntensity = 2.f;
		Ref<Texture2D> DirtTexture;
	};

	struct AmbientOcclusionSettings
	{
		bool Enable = true;
		float Intensity = 2.f;
		bool HalfRes = false;
		float Radius = 0.5f;
		float Bias = 0.f;
	};

	struct QualitySettings
	{
		float RendererScale = 1.f;
	};

	struct SceneRendererSettings
	{
		ShadowSettings ShadowSettings;
		BloomSettings BloomSettings;
		AmbientOcclusionSettings AOSettings;
		PostProcessingSettings PostProcessingSettings;
		DebugView DebugView = DebugView::NONE;
		QualitySettings Quality;
	};


	struct CameraData
	{
		Matrix4 View;
		Matrix4 InverseView;
		Matrix4 Projection;
		Matrix4 InverseProjection;
		Matrix4 ViewProjection;
		Matrix4 InverseViewProjection;
		Matrix4 RotationView;
		Vector3 Position;
		float NearClip;
		float FarClip;
	};

	struct RendererData
	{
		Vector2 ViewportSize;
		Vector2 InverseViewportSize;
		Vector4i ViewportTilesCount;	// xy - per width and height, z - all tiles, w - empty
		float EnvironmentIntensity;
		float EnvironmentLOD;
		int32 DebugShadowCascades;
		int32 DebugLightComplexity;
	};

	struct LightData
	{
		DirectionalLight DirectionalLights[ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT];
		uint32 DirectionalLightCount = 0;

		PointLight PointLights[ShaderDef::MAX_POINT_LIGHT_COUNT];
		uint32 PointLightCount = 0;

		SpotLight SpotLights[ShaderDef::MAX_SPOT_LIGHT_COUNT];
		uint32 SpotLightCount = 0;
	};

	struct TileVisibleLights
	{
		uint32 Count;
		uint32 LightIndices[ShaderDef::MAX_POINT_LIGHT_COUNT_PER_TILE];
	};

	struct ShadowsData
	{
		Matrix4 DirLightViewProjection[ShaderDef::SHADOW_CASCADES_COUNT];
		Vector4 CascadePlanes[ShaderDef::SHADOW_CASCADES_COUNT];
		float MaxDistance = 200.f;
		float FadeOut = 10.f;
		float CascadeBlendDistance = 0.5f;
		int SoftShadows = true;
	};

	struct AOData
	{
		Vector4 SamplesKernel[64];
		Vector4 KernelNoise[16];
		float Intensity;
		float Radius;
		float Bias;
	};

	struct SceneRendererStatistics
	{
		Time GPUTime;
		Time DirShadowMapPass;
		Time GBufferPass;
		Time LightCullingPass;
		Time SSAOPass;
		Time SSAODenoisePass;
		Time DeferredLightingPass;
		Time SkyboxPass;
		Time BloomPass;
		Time SceneCompositePass;
		Time JumpFloodPass;
		Time Render2DPass;
		Time FXAAPass;
		PipelineStatistics PipelineStats;

		uint32 Meshes;
		uint32 Instances;
		uint32 AnimMeshes;
	};

	using Render2DCallback = std::function<void()>;
	using OnViewportResizeCallback = std::function<void(uint32, uint32)>;

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

		void OnViewportResize(uint32 width, uint32 height);

		void BeginScene(const CameraInfo& cameraInfo);
		void EndScene();

		void Submit(const Ref<StaticMesh>& mesh, const Matrix4& transform = Matrix4::Identity());
		void SubmitLightEnvironment(const LightEnvironment& lightEnv);

		void SubmitSelectionContext(const Ref<StaticMesh>& mesh, const Matrix4& transform = Matrix4::Identity());

		void SetOnRender2DCallback(const Render2DCallback& callback);
		void SetOnViewportResizeCallback(const OnViewportResizeCallback& callback);

		Ref<Texture2D> GetFinalImage();
		Ref<Texture2D> GetShadowMap();
		Ref<Texture2D> GetBloomTexture() { return m_BloomTexture; }

		Ref<RenderPass> GetGBufferPass() { return m_GBufferPass; }
		Ref<RenderPass> GetRender2DPass() { return m_Render2DPass; }
		Ref<RenderPass> GetSSAOPass() { return m_SSAODenoisePass; }
		Ref<Pipeline> GetSkyboxPipeline() { return m_SkyboxPipeline; }

		void ApplySettings();

	private:
		void DirShadowMapPass();
		void GBufferPass();
		void SSAOPass();
		void LightCullingPass();
		void LightingPass();
		void SkyboxPass();
		void BloomPass();
		void SceneCompositePass();
		void JumpFloodPass();
		void Render2DPass();
		void FXAAPass();

		void CalculateInstanceTransforms();
		void CalculateCascadeLightSpaces(DirectionalLight& light);

		void ResetStats();

		void SubmitStaticMesh(DrawListStatic& list, const Ref<StaticMesh>& mesh, const Matrix4& transform);
		void SubmitAnimMesh(DrawListAnim& list, const Ref<StaticMesh>& mesh, const Ref<Animator>& animator, const Matrix4& transform);

	private:
		const uint32 m_ShadowMapResolution = 2048;

		const float m_OutlineWidth = 1.3f;
		const Vector4 m_OutlineColor = { 1.f, 0.5f, 0.f, 1.f };

	private:
		// DrawLists
		DrawListStatic m_StaticGeometryList;
		DrawListAnim m_AnimGeometryList;

		DrawListStatic m_SelectStaticGeometryList;
		DrawListAnim m_SelectAnimGeometryList;

		// Render Passes
		Ref<RenderPass> m_DirShadowMapPass;
		Ref<Pipeline> m_DirShadowMapStaticPipeline;
		Ref<Pipeline> m_DirShadowMapAnimPipeline;

		Ref<RenderPass> m_GBufferPass;
		Ref<Pipeline> m_StaticGeometryPipeline;
		Ref<Pipeline> m_AnimGeometryPipeline;

		Ref<ComputePass> m_LightCullingPass;
		Ref<ComputePipeline> m_LightCullingPipeline;

		Ref<RenderPass> m_DeferredLightingPass;
		Ref<Pipeline> m_DeferredLightingPipeline;

		Ref<RenderPass> m_SkyboxPass;
		Ref<Pipeline> m_SkyboxPipeline;

		Ref<Texture2D> m_BloomTexture;
		Ref<ComputePass> m_BloomPass;
		Ref<ComputePipeline> m_BloomDownsample;
		Ref<ComputePipeline> m_BloomUpsample;
		std::vector<Ref<Material>> m_BloomMaterials;

		Ref<RenderPass> m_SSAOPass;
		Ref<Pipeline> m_SSAOPipeline;
		Ref<RenderPass> m_SSAODenoisePass;
		Ref<Pipeline> m_SSAODenoisePipeline;

		Ref<RenderPass> m_SceneCompositePass;
		Ref<Pipeline> m_SceneCompositePipeline;
		Ref<Material> m_SceneCompositeMaterial;

		Ref<RenderPass> m_JumpFloodSilhouettePass;
		Ref<Pipeline> m_JFSilhouetteStaticPipeline;
		Ref<Pipeline> m_JFSilhouetteAnimPipeline;
		Ref<RenderPass> m_JumpFloodInitPass;
		Ref<Pipeline> m_JumpFloodInitPipeline;
		Ref<RenderPass> m_JumpFloodPasses[2];
		Ref<Pipeline> m_JumpFloodPipelines[2];
		Ref<Material> m_JumpFloodMaterial;
		Ref<RenderPass> m_JumpFloodCompositePass;
		Ref<Pipeline> m_JumpFloodCompositePipeline;
		Ref<Material> m_JumpFloodCompositeMaterial;

		Ref<RenderPass> m_Render2DPass;
		Render2DCallback m_Render2DCallback;

		Ref<Texture2D> m_PostProcessTexture;
		Ref<ComputePass> m_FXAAPass;
		Ref<ComputePipeline> m_FXAAPipeline;

		// CPU Data
		CameraData m_CameraData;
		RendererData m_RendererData;
		LightData m_LightData;
		ShadowsData m_ShadowsData;
		AOData m_AmbientOcclusionData;
		uint32 m_BonesDataOffset;

		// GPU Data
		Ref<UniformBuffer> m_CameraUBO;
		Ref<UniformBuffer> m_RendererUBO;
		Ref<StorageBuffer> m_LightSBO;
		Ref<StorageBuffer> m_VisibleLightsSBO;
		Ref<UniformBuffer> m_ShadowsUBO;
		Ref<UniformBuffer> m_AmbientOcclusionUBO;
		Ref<TextureView> m_ShadowMapSampler;

		DynamicGPUBuffer<StorageBuffer> m_BonesSBO;
		DynamicGPUBuffer<VertexBuffer> m_TransformsStorage;

		// Other
		Vector2u m_ViewportSize = { 1, 1 };
		Vector2u m_OriginalViewportSize = { 1, 1 };
		OnViewportResizeCallback m_ViewportResizeCallback;

		Ref<RenderCommandBuffer> m_RenderCommandBuffer;
		Ref<GPUProfiler> m_Profiler;
		SceneRendererStatistics m_Statistics;
		SceneRendererSettings m_Settings;
	};
}
