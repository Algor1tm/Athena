#include "SceneRenderer.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"
#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	Ref<SceneRenderer> SceneRenderer::Create()
	{
		Ref<SceneRenderer> renderer = Ref<SceneRenderer>::Create();
		renderer->Init();

		return renderer;
	}

	SceneRenderer::~SceneRenderer()
	{
		Shutdown();
	}

	void SceneRenderer::Init()
	{
		GPUProfilerCreateInfo profilerInfo;
		profilerInfo.Name = "SceneRendererProfiler";
		profilerInfo.RenderCommandBuffer = Renderer::GetRenderCommandBuffer();
		profilerInfo.MaxTimestampsCount = 16;
		profilerInfo.MaxPipelineQueriesCount = 1;
		m_Profiler = GPUProfiler::Create(profilerInfo);

		m_StaticGeometryList = DrawList(false);
		m_AnimGeometryList = DrawList(true);

		m_CameraUBO = UniformBuffer::Create("CameraUBO", sizeof(CameraData));
		m_RendererUBO = UniformBuffer::Create("RendererUBO", sizeof(RendererData));
		m_LightSBO = StorageBuffer::Create("LightSBO", sizeof(LightData));
		m_ShadowsUBO = UniformBuffer::Create("ShadowsUBO", sizeof(ShadowsData));

		uint64 maxNumBones = ShaderDef::MAX_NUM_BONES_PER_MESH * ShaderDef::MAX_NUM_ANIMATED_MESHES;
		m_BonesSBO = StorageBuffer::Create("BonesSBO", maxNumBones * sizeof(Matrix4));

		m_BonesData = std::vector<Matrix4>(maxNumBones);
		m_BonesDataOffset = 0;

		// Dir ShadowMap Pass
		{
			for (uint32 i = 0; i < ShaderDef::SHADOW_CASCADES_COUNT; ++i)
				m_ShadowsData.DirLightViewProjection[i] = Matrix4::Identity();

			Texture2DCreateInfo shadowMapInfo;
			shadowMapInfo.Name = "DirShadowMap";
			shadowMapInfo.Format = ImageFormat::DEPTH32F;
			shadowMapInfo.Usage = ImageUsage(ImageUsage::ATTACHMENT | ImageUsage::SAMPLED);
			shadowMapInfo.Width = m_ShadowMapResolution;
			shadowMapInfo.Height = m_ShadowMapResolution;
			shadowMapInfo.Layers = ShaderDef::SHADOW_CASCADES_COUNT;
			shadowMapInfo.MipLevels = 1;
			shadowMapInfo.SamplerInfo.MinFilter = TextureFilter::NEAREST;
			shadowMapInfo.SamplerInfo.MagFilter = TextureFilter::NEAREST;
			shadowMapInfo.SamplerInfo.Wrap = TextureWrap::CLAMP_TO_BORDER;

			RenderPassCreateInfo passInfo;
			passInfo.Name = "DirShadowMapPass";
			passInfo.Width = m_ShadowMapResolution;
			passInfo.Height = m_ShadowMapResolution;
			passInfo.Layers = ShaderDef::SHADOW_CASCADES_COUNT;
			passInfo.DebugColor = { 0.7f, 0.8f, 0.7f, 1.f };

			Ref<Texture2D> shadowMap = Texture2D::Create(shadowMapInfo);
			RenderPassAttachment output = RenderPassAttachment(shadowMap);
			output.LoadOp = AttachmentLoadOp::CLEAR;
			output.DepthClearColor = 1.f;

			m_DirShadowMapPass = RenderPass::Create(passInfo);
			m_DirShadowMapPass->SetOutput(output);
			m_DirShadowMapPass->Bake();


			PipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "DirShadowMapStatic";
			pipelineInfo.RenderPass = m_DirShadowMapPass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("DirShadowMap_Static");
			pipelineInfo.VertexLayout = StaticVertex::GetLayout();
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::FRONT;
			pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
			pipelineInfo.BlendEnable = false;

			m_DirShadowMapStaticPipeline = Pipeline::Create(pipelineInfo);
			m_DirShadowMapStaticPipeline->SetInput("u_ShadowsData", m_ShadowsUBO);
			m_DirShadowMapStaticPipeline->Bake();

			pipelineInfo.Name = "DirShadowMapAnim";
			pipelineInfo.RenderPass = m_DirShadowMapPass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("DirShadowMap_Anim");
			pipelineInfo.VertexLayout = AnimVertex::GetLayout();
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::FRONT;
			pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
			pipelineInfo.BlendEnable = false;

			m_DirShadowMapAnimPipeline = Pipeline::Create(pipelineInfo);
			m_DirShadowMapAnimPipeline->SetInput("u_ShadowsData", m_ShadowsUBO);
			m_DirShadowMapAnimPipeline->SetInput("u_BonesData", m_BonesSBO);
			m_DirShadowMapAnimPipeline->Bake();

			TextureSamplerCreateInfo samplerInfo;
			samplerInfo.MinFilter = TextureFilter::LINEAR;
			samplerInfo.MagFilter = TextureFilter::LINEAR;
			samplerInfo.Wrap = TextureWrap::CLAMP_TO_BORDER;
			samplerInfo.Compare = TextureCompareOperator::LEQUAL;
			m_ShadowMapSampler = Texture2D::Create(shadowMap->GetImage(), samplerInfo);
		}

		// Geometry Pass
		{
			RenderPassCreateInfo passInfo;
			passInfo.Name = "GeometryPass";
			passInfo.InputPass = m_DirShadowMapPass;
			passInfo.Width = m_ViewportSize.x;
			passInfo.Height = m_ViewportSize.y;
			passInfo.DebugColor = { 0.4f, 0.8f, 0.2f, 1.f };

			m_GeometryPass = RenderPass::Create(passInfo);
			m_GeometryPass->SetOutput({ "GeometryHDRColor", ImageFormat::RGBA16F });
			m_GeometryPass->SetOutput({ "GeometryDepth", ImageFormat::DEPTH24STENCIL8 });
			m_GeometryPass->Bake();


			PipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "StaticGeometryPipeline";
			pipelineInfo.RenderPass = m_GeometryPass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("PBR_Static");
			pipelineInfo.VertexLayout = StaticVertex::GetLayout();
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
			pipelineInfo.BlendEnable = true;

			m_StaticGeometryPipeline = Pipeline::Create(pipelineInfo);

			m_StaticGeometryPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_StaticGeometryPipeline->SetInput("u_LightData", m_LightSBO);
			m_StaticGeometryPipeline->SetInput("u_RendererData", m_RendererUBO);
			m_StaticGeometryPipeline->SetInput("u_ShadowsData", m_ShadowsUBO);
			m_StaticGeometryPipeline->SetInput("u_DirShadowMap", m_DirShadowMapPass->GetOutput("DirShadowMap"));
			m_StaticGeometryPipeline->SetInput("u_DirShadowMapShadow", m_ShadowMapSampler);
			m_StaticGeometryPipeline->SetInput("u_BRDF_LUT", Renderer::GetBRDF_LUT());
			m_StaticGeometryPipeline->SetInput("u_EnvironmentMap", Renderer::GetBlackTextureCube());
			m_StaticGeometryPipeline->SetInput("u_IrradianceMap", Renderer::GetBlackTextureCube());
			m_StaticGeometryPipeline->Bake();


			pipelineInfo.Name = "AnimGeometryPipeline";
			pipelineInfo.RenderPass = m_GeometryPass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("PBR_Anim");
			pipelineInfo.VertexLayout = AnimVertex::GetLayout();
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
			pipelineInfo.BlendEnable = true;

			m_AnimGeometryPipeline = Pipeline::Create(pipelineInfo);

			m_AnimGeometryPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_AnimGeometryPipeline->SetInput("u_LightData", m_LightSBO);
			m_AnimGeometryPipeline->SetInput("u_RendererData", m_RendererUBO);
			m_AnimGeometryPipeline->SetInput("u_BonesData", m_BonesSBO);
			m_AnimGeometryPipeline->SetInput("u_BRDF_LUT", Renderer::GetBRDF_LUT());
			m_AnimGeometryPipeline->SetInput("u_EnvironmentMap", Renderer::GetBlackTextureCube());
			m_AnimGeometryPipeline->SetInput("u_IrradianceMap", Renderer::GetBlackTextureCube());
			m_AnimGeometryPipeline->Bake();


			pipelineInfo.Name = "SkyboxPipeline";
			pipelineInfo.RenderPass = m_GeometryPass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("Skybox");
			pipelineInfo.VertexLayout = { 
				{ ShaderDataType::Float3, "a_Position" } };
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
			pipelineInfo.BlendEnable = false;

			m_SkyboxPipeline = Pipeline::Create(pipelineInfo);
			m_SkyboxPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_SkyboxPipeline->SetInput("u_RendererData", m_RendererUBO);
			m_SkyboxPipeline->SetInput("u_EnvironmentMap", Renderer::GetBlackTextureCube());
			m_SkyboxPipeline->Bake();
		}

		// Composite Pass
		{
			RenderPassCreateInfo passInfo;
			passInfo.Name = "SceneCompositePass";
			passInfo.InputPass = m_GeometryPass;
			passInfo.Width = m_ViewportSize.x;
			passInfo.Height = m_ViewportSize.y;
			passInfo.DebugColor = { 0.2f, 0.5f, 1.0f, 1.f };

			m_CompositePass = RenderPass::Create(passInfo);
			m_CompositePass->SetOutput({ "SceneCompositeColor", ImageFormat::RGBA8 });
			m_CompositePass->Bake();


			PipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "SceneCompositePipeline";
			pipelineInfo.RenderPass = m_CompositePass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("SceneComposite");
			pipelineInfo.VertexLayout = {
				{ ShaderDataType::Float2, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoords"} };

			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompare = DepthCompare::NONE;
			pipelineInfo.BlendEnable = false;

			m_CompositePipeline = Pipeline::Create(pipelineInfo);

			m_CompositePipeline->SetInput("u_SceneHDRColor", m_GeometryPass->GetOutput("GeometryHDRColor"));
			m_CompositePipeline->SetInput("u_RendererData", m_RendererUBO);
			m_CompositePipeline->Bake();
		}

		// Renderer2D Pass
		{
			RenderPassCreateInfo passInfo;
			passInfo.Name = "Renderer2DPass";
			passInfo.InputPass = m_CompositePass;
			passInfo.Width = m_ViewportSize.x;
			passInfo.Height = m_ViewportSize.y;
			passInfo.DebugColor = { 0.9f, 0.1f, 0.2f, 1.f };

			RenderPassAttachment colorOutput = m_CompositePass->GetOutput("SceneCompositeColor");
			RenderPassAttachment depthOutput = m_GeometryPass->GetOutput("GeometryDepth");

			m_Renderer2DPass = RenderPass::Create(passInfo);
			m_Renderer2DPass->SetOutput(colorOutput);
			m_Renderer2DPass->SetOutput(depthOutput);
			m_Renderer2DPass->Bake();

			m_SceneRenderer2D = SceneRenderer2D::Create(m_Renderer2DPass);
		}
	}

	void SceneRenderer::Shutdown()
	{
		m_SceneRenderer2D.Release();
	}

	Ref<Image> SceneRenderer::GetFinalImage()
	{
		return m_CompositePass->GetOutput("SceneCompositeColor")->GetImage();
	}

	Ref<Image> SceneRenderer::GetShadowMap()
	{
		return m_DirShadowMapPass->GetDepthOutput()->GetImage();
	}

	void SceneRenderer::OnViewportResize(uint32 width, uint32 height)
	{
		m_ViewportSize = { width, height };

		m_GeometryPass->Resize(width, height);
		m_StaticGeometryPipeline->SetViewport(width, height);
		m_AnimGeometryPipeline->SetViewport(width, height);
		m_SkyboxPipeline->SetViewport(width, height);

		m_CompositePass->Resize(width, height);
		m_CompositePipeline->SetViewport(width, height);

		m_Renderer2DPass->Resize(width, height);
		m_SceneRenderer2D->OnViewportResize(width, height);
	}

	void SceneRenderer::Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Matrix4& transform)
	{
		DrawCall drawCall;
		drawCall.VertexBuffer = vertexBuffer;
		drawCall.Transform = transform;
		drawCall.Material = material;
		drawCall.BonesOffset = 0;

		m_StaticGeometryList.Push(drawCall);
	}

	void SceneRenderer::Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform)
	{
		if (m_BonesDataOffset >= m_BonesData.size())
		{
			ATN_CORE_WARN_TAG("Renderer", "Attempt to submit more than {} animated meshes", ShaderDef::MAX_NUM_ANIMATED_MESHES);
		}

		DrawCall drawCall;
		drawCall.VertexBuffer = vertexBuffer;
		drawCall.Transform = transform;
		drawCall.Material = material;
		drawCall.BonesOffset = m_BonesDataOffset;

		const auto& bones = animator->GetBoneTransforms();
		memcpy(&m_BonesData[m_BonesDataOffset], bones.data(), bones.size() * sizeof(Matrix4));

		m_BonesDataOffset += bones.size();

		m_AnimGeometryList.Push(drawCall);
	}

	void SceneRenderer::SubmitLightEnvironment(const LightEnvironment& lightEnv)
	{
		m_LightData.DirectionalLightCount = lightEnv.DirectionalLights.size();

		if (m_LightData.DirectionalLightCount > ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT)
		{
			ATN_CORE_WARN_TAG("Renderer", "Attempt to submit more than {} DirectionalLights!", ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT);
			m_LightData.DirectionalLightCount = ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT;
		}

		m_LightData.PointLightCount = lightEnv.PointLights.size();
		if (m_LightData.PointLightCount > ShaderDef::MAX_POINT_LIGHT_COUNT)
		{
			ATN_CORE_WARN_TAG("Renderer", "Attempt to submit more than {} PointLights!", ShaderDef::MAX_POINT_LIGHT_COUNT);
			m_LightData.PointLightCount = ShaderDef::MAX_POINT_LIGHT_COUNT;
		}

		// For now pick first directional light that cast shadows, others won't cast shadows
		bool castsShadows = false;
		for (uint32 i = 0; i < m_LightData.DirectionalLightCount; ++i)
		{
			DirectionalLight& light = m_LightData.DirectionalLights[i];
			light = lightEnv.DirectionalLights[i];
			light.Direction.Normalize();

			if (castsShadows && light.CastShadows)
			{
				ATN_CORE_WARN_TAG("Renderer", "Attempt to submit more than 1 DirectionalLight, that casts shadows!");
				light.CastShadows = false;
			}

			if (light.CastShadows)
			{
				CalculateCascadeLightSpaces(light);
				castsShadows = true;
			}
		}

		for (uint32 i = 0; i < m_LightData.PointLightCount; ++i)
			m_LightData.PointLights[i] = lightEnv.PointLights[i];

		m_RendererData.EnvironmentIntensity = lightEnv.EnvironmentMapIntensity;
		m_RendererData.EnvironmentLOD = lightEnv.EnvironmentMapLOD;

		if (lightEnv.EnvironmentMap)
		{
			auto irradianceMap = lightEnv.EnvironmentMap->GetIrradianceTexture();
			auto environmentMap = lightEnv.EnvironmentMap->GetEnvironmentTexture();

			m_StaticGeometryPipeline->SetInput("u_IrradianceMap", irradianceMap);
			m_StaticGeometryPipeline->SetInput("u_EnvironmentMap", environmentMap);
			m_AnimGeometryPipeline->SetInput("u_IrradianceMap", irradianceMap);
			m_AnimGeometryPipeline->SetInput("u_EnvironmentMap", environmentMap);
			m_SkyboxPipeline->SetInput("u_EnvironmentMap", environmentMap);
		}
		else
		{
			m_StaticGeometryPipeline->SetInput("u_IrradianceMap", Renderer::GetBlackTextureCube());
			m_StaticGeometryPipeline->SetInput("u_EnvironmentMap", Renderer::GetBlackTextureCube());
			m_AnimGeometryPipeline->SetInput("u_IrradianceMap", Renderer::GetBlackTextureCube());
			m_AnimGeometryPipeline->SetInput("u_EnvironmentMap", Renderer::GetBlackTextureCube());
			m_SkyboxPipeline->SetInput("u_EnvironmentMap", Renderer::GetBlackTextureCube());
		}
	}

	void SceneRenderer::BeginScene(const CameraInfo& cameraInfo)
	{
		m_RenderCommandBuffer = Renderer::GetRenderCommandBuffer();

		m_CameraData.View = cameraInfo.ViewMatrix;
		m_CameraData.Projection = cameraInfo.ProjectionMatrix;
		m_CameraData.ViewProjection = cameraInfo.ViewMatrix * cameraInfo.ProjectionMatrix;
		m_CameraData.RotationView = cameraInfo.ViewMatrix.AsMatrix3();
		m_CameraData.Position = Math::AffineInverse(cameraInfo.ViewMatrix)[3];
		m_CameraData.NearClip = cameraInfo.NearClip;
		m_CameraData.FarClip = cameraInfo.FarClip;

		m_RendererData.Exposure = m_Settings.LightEnvironmentSettings.Exposure;
		m_RendererData.Gamma = m_Settings.LightEnvironmentSettings.Gamma;
		m_RendererData.DebugShadowCascades = m_Settings.DebugView == DebugView::SHADOW_CASCADES ? 1 : 0;

		m_ShadowsData.MaxDistance = m_Settings.ShadowSettings.MaxDistance;
		m_ShadowsData.FadeOut = m_Settings.ShadowSettings.FadeOut;
		m_ShadowsData.CascadeBlendDistance = m_Settings.ShadowSettings.CascadeBlendDistance;
		m_ShadowsData.SoftShadows = m_Settings.ShadowSettings.SoftShadows;

		m_BonesDataOffset = 0;
	}

	void SceneRenderer::EndScene()
	{
		m_Profiler->Reset();
		m_Profiler->BeginPipelineStatsQuery();

		m_StaticGeometryList.Sort();
		m_AnimGeometryList.Sort();

		Renderer::Submit([this]()
		{
			m_CameraUBO->RT_SetData(&m_CameraData, sizeof(CameraData));
			m_RendererUBO->RT_SetData(&m_RendererData, sizeof(RendererData));
			m_LightSBO->RT_SetData(&m_LightData, sizeof(LightData));
			m_ShadowsUBO->RT_SetData(&m_ShadowsData, sizeof(ShadowsData));
			m_BonesSBO->RT_SetData(m_BonesData.data(), m_BonesDataOffset * sizeof(Matrix4));
		});

		DirShadowMapPass();
		GeometryPass();
		SceneCompositePass();

		m_Statistics.PipelineStats = m_Profiler->EndPipelineStatsQuery();

		m_StaticGeometryList.Clear();
		m_AnimGeometryList.Clear();
	}

	void SceneRenderer::DirShadowMapPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		m_DirShadowMapPass->Begin(commandBuffer);

		Renderer::BeginDebugRegion(commandBuffer, "StaticGeometry", { 0.8f, 0.4f, 0.2f, 1.f });
		if (m_DirShadowMapStaticPipeline->Bind(commandBuffer))
		{
			m_StaticGeometryList.Flush(m_DirShadowMapStaticPipeline, false);
		}
		Renderer::EndDebugRegion(commandBuffer);


		Renderer::BeginDebugRegion(commandBuffer, "AnimatedGeometry", { 0.8f, 0.4f, 0.8f, 1.f });
		if (m_DirShadowMapAnimPipeline->Bind(commandBuffer))
		{
			m_AnimGeometryList.Flush(m_DirShadowMapAnimPipeline, false);
		}
		Renderer::EndDebugRegion(commandBuffer);

		m_DirShadowMapPass->End(commandBuffer);
		m_Statistics.DirShadowMapPass = m_Profiler->EndTimeQuery();
	}

	void SceneRenderer::GeometryPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		m_GeometryPass->Begin(commandBuffer);


		Renderer::BeginDebugRegion(commandBuffer, "StaticGeometry", { 0.8f, 0.4f, 0.2f, 1.f });
		if (m_StaticGeometryPipeline->Bind(commandBuffer))
		{
			m_StaticGeometryList.Flush(m_StaticGeometryPipeline);
		}
		Renderer::EndDebugRegion(commandBuffer);
			

		Renderer::BeginDebugRegion(commandBuffer, "AnimatedGeometry", { 0.8f, 0.4f, 0.8f, 1.f });
		if (m_AnimGeometryPipeline->Bind(commandBuffer))
		{
			m_AnimGeometryList.Flush(m_AnimGeometryPipeline);
		}
		Renderer::EndDebugRegion(commandBuffer);


		Renderer::BeginDebugRegion(commandBuffer, "Skybox", { 0.3f, 0.6f, 0.6f, 1.f });
		if (m_SkyboxPipeline->Bind(commandBuffer))
		{
			Renderer::RenderNDCCube(commandBuffer, m_SkyboxPipeline);
		}
		Renderer::EndDebugRegion(commandBuffer);


		m_GeometryPass->End(commandBuffer);
		m_Statistics.GeometryPass = m_Profiler->EndTimeQuery();
	}

	void SceneRenderer::SceneCompositePass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		m_CompositePass->Begin(commandBuffer);

		if (m_CompositePipeline->Bind(commandBuffer))
		{
			Renderer::RenderFullscreenQuad(commandBuffer, m_CompositePipeline);
		}

		m_CompositePass->End(commandBuffer);
		m_Statistics.SceneCompositePass = m_Profiler->EndTimeQuery();
	}

	void SceneRenderer::CalculateCascadeLightSpaces(DirectionalLight& light)
	{
		float cameraNear = m_CameraData.NearClip;
		float cameraFar = m_CameraData.FarClip;

		const float splitWeight = m_Settings.ShadowSettings.CascadeSplit;

		// Split cascades
		for (uint32 i = 0; i < ShaderDef::SHADOW_CASCADES_COUNT; ++i)
		{
			float percent = (i + 1) / float(ShaderDef::SHADOW_CASCADES_COUNT);

			float log = cameraNear * Math::Pow(cameraFar / cameraNear, percent);
			float uniform = Math::Lerp(cameraNear, cameraFar, percent);
			float split = Math::Lerp(uniform, log, splitWeight);

			m_ShadowsData.Cascades[i].SplitDepth = split;
		}

		Matrix4 invCamera = Math::Inverse(m_CameraData.View * m_CameraData.Projection);

		float lastSplit = 0.f;

		// Build light space matrices
		for (uint32 layer = 0; layer < ShaderDef::SHADOW_CASCADES_COUNT; ++layer)
		{
			// convert to range (0, 1]
			float split = (m_ShadowsData.Cascades[layer].SplitDepth - cameraNear) / (cameraFar - cameraNear);

			std::array<Vector3, 8> frustumCorners = {
				//Near face
				Vector3{  1.0f, -1.0f, -1.0f },
				Vector3{ -1.0f, -1.0f, -1.0f },
				Vector3{  1.0f,  1.0f, -1.0f },
				Vector3{ -1.0f,  1.0f, -1.0f },

				//Far face
				Vector3{  1.0f, -1.0f, 1.0f },
				Vector3{ -1.0f, -1.0f, 1.0f },
				Vector3{  1.0f,  1.0f, 1.0f },
				Vector3{ -1.0f,  1.0f, 1.0f },
			};

			// Transform frustum to world space
			for (uint32 j = 0; j < frustumCorners.size(); ++j)
			{
				Vector4 cornerWorldSpace = Vector4(frustumCorners[j], 1.f) * invCamera;
				frustumCorners[j] = cornerWorldSpace / cornerWorldSpace.w;
			}

			for (uint32_t j = 0; j < 4; ++j)
			{
				Vector3 dist = frustumCorners[j + 4] - frustumCorners[j];
				frustumCorners[j + 4] = frustumCorners[j] + (dist * split);
				frustumCorners[j] = frustumCorners[j] + (dist * lastSplit);
			}

			Vector3 frustumCenter = Vector3(0.f);
			for (const auto& corner : frustumCorners)
				frustumCenter += corner;

			frustumCenter /= frustumCorners.size();

			// Build bounding box
			float radius = 0.0f;
			for (uint32 j = 0; j < frustumCorners.size(); ++j)
			{
				float distance = (frustumCorners[j] - frustumCenter).Length();
				radius = Math::Max(radius, distance);
			}
			radius = Math::Ceil(radius * 16.0f) / 16.0f;

			Vector3 maxExtents = Vector3(radius);
			Vector3 minExtents = -maxExtents;

			minExtents.z += m_Settings.ShadowSettings.NearPlaneOffset;
			maxExtents.z += m_Settings.ShadowSettings.FarPlaneOffset;

			Matrix4 lightView = Math::LookAt(frustumCenter + light.Direction.GetNormalized() * minExtents.z, frustumCenter, Vector3::Up());
			Matrix4 lightProjection = Math::Ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.f, maxExtents.z - minExtents.z);

			Matrix4 lightSpace = lightView * lightProjection;

			// Error reducing step
			Vector4 shadowOrigin = Vector4(0.f, 0.f, 0.f, 1.f);
			shadowOrigin = shadowOrigin * lightSpace;
			shadowOrigin = shadowOrigin * (m_ShadowMapResolution / 2.f);

			Vector4 roundedOrigin = Math::Round(shadowOrigin);
			Vector4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * (2.f / m_ShadowMapResolution);
			roundOffset.z = 0.f;
			roundOffset.w = 0.f;

			lightProjection[3] += roundOffset;

			m_ShadowsData.DirLightViewProjection[layer] = lightView * lightProjection;
			m_ShadowsData.DirLightView[layer] = lightView;

			m_ShadowsData.Cascades[layer].Near = cameraNear * split;
			m_ShadowsData.Cascades[layer].Far = cameraFar * split;

			lastSplit = split;
		}
	}
}
