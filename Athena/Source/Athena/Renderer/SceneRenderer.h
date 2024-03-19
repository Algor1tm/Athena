#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Camera.h"
#include "Athena/Renderer/GPUProfiler.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/RenderPass.h"
#include "Athena/Renderer/ComputePass.h"
#include "Athena/Renderer/ComputePipeline.h"
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
		FXAA = 1
	};

	enum class DebugView
	{
		NONE = 0,
		SHADOW_CASCADES = 1,
		DEPTH
	};

	enum class TonemapMode
	{
		NONE,
		ACES,
		EXPOSURE,
	};

	struct PostProcessingSettings
	{
		TonemapMode TonemapMode = TonemapMode::ACES;
		float Exposure = 1.f;
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
		float Intensity = 1;
		float Threshold = 1.5;
		float Knee = 0.1;
		float DirtIntensity = 2;
		Ref<Texture2D> DirtTexture;
	};

	struct SceneRendererSettings
	{
		ShadowSettings ShadowSettings;
		BloomSettings BloomSettings;
		PostProcessingSettings PostProcessingSettings;
		DebugView DebugView = DebugView::NONE;
	};


	struct CameraData
	{
		Matrix4 View;
		Matrix4 Projection;
		Matrix4 ViewProjection;
		Matrix4 RotationView;
		Vector3 Position;
		float NearClip;
		float FarClip;
	};

	struct RendererData
	{
		Vector2 ViewportSize;
		Vector2 InverseViewportSize;
		float EnvironmentIntensity;
		float EnvironmentLOD ;
		int32 DebugShadowCascades;
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

	struct ShadowsData
	{
		Matrix4 DirLightViewProjection[ShaderDef::SHADOW_CASCADES_COUNT];
		Vector4 CascadePlanes[ShaderDef::SHADOW_CASCADES_COUNT];
		float MaxDistance = 200.f;
		float FadeOut = 10.f;
		float CascadeBlendDistance = 0.5f;
		int SoftShadows = true;
	};

	struct SceneRendererStatistics
	{
		Time GPUTime;
		Time DirShadowMapPass;
		Time GeometryPass;
		Time BloomPass;
		Time SceneCompositePass;
		Time Render2DPass;
		Time FXAAPass;
		PipelineStatistics PipelineStats;
	};

	using Render2DCallback = std::function<void()>;

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

		void Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Matrix4& transform = Matrix4::Identity());
		void Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform = Matrix4::Identity());
		void SubmitLightEnvironment(const LightEnvironment& lightEnv);

		void OnRender2D(const Render2DCallback& callback);

		Ref<Texture2D> GetFinalImage();
		Ref<Texture2D> GetShadowMap();
		Ref<Texture2D> GetBloomTexture() { return m_BloomTexture; }
		Ref<RenderPass> GetRender2DPass() { return m_Render2DPass; }

	private:
		void DirShadowMapPass();
		void GeometryPass();
		void BloomPass();
		void SceneCompositePass();
		void Render2DPass();
		void FXAAPass();

		void CalculateCascadeLightSpaces(const DirectionalLight& light);

	private:
		const uint32 m_ShadowMapResolution = 2048;

	private:
		// DrawLists
		DrawList m_StaticGeometryList;
		DrawList m_AnimGeometryList;

		// Render Passes
		Ref<RenderPass> m_DirShadowMapPass;
		Ref<Pipeline> m_DirShadowMapStaticPipeline;
		Ref<Pipeline> m_DirShadowMapAnimPipeline;

		Ref<RenderPass> m_GeometryPass;
		Ref<Pipeline> m_StaticGeometryPipeline;
		Ref<Pipeline> m_AnimGeometryPipeline;
		Ref<Pipeline> m_SkyboxPipeline;

		Ref<Texture2D> m_BloomTexture;
		Ref<ComputePass> m_BloomPass;
		Ref<ComputePipeline> m_BloomDownsample;
		Ref<ComputePipeline> m_BloomUpsample;
		std::vector<Ref<Material>> m_BloomMaterials;

		Ref<RenderPass> m_CompositePass;
		Ref<Pipeline> m_CompositePipeline;
		Ref<Material> m_CompositeMaterial;

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
		std::vector<Matrix4> m_BonesData;
		uint32 m_BonesDataOffset;

		// GPU Data
		Ref<UniformBuffer> m_CameraUBO;
		Ref<UniformBuffer> m_RendererUBO;
		Ref<StorageBuffer> m_LightSBO;
		Ref<UniformBuffer> m_ShadowsUBO;
		Ref<StorageBuffer> m_BonesSBO;
		Ref<Texture2D> m_ShadowMapSampler;

		// Other
		Vector2u m_ViewportSize = { 1, 1 };

		Ref<RenderCommandBuffer> m_RenderCommandBuffer;
		Ref<GPUProfiler> m_Profiler;
		SceneRendererStatistics m_Statistics;
		SceneRendererSettings m_Settings;
	};
}
