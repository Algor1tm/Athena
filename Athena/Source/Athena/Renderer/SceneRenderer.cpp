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

		// DIR SHADOW MAP PASS
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
			shadowMapInfo.GenerateMipLevels = false;
			shadowMapInfo.SamplerInfo.MinFilter = TextureFilter::NEAREST;
			shadowMapInfo.SamplerInfo.MagFilter = TextureFilter::NEAREST;
			shadowMapInfo.SamplerInfo.Wrap = TextureWrap::CLAMP_TO_BORDER;

			RenderPassCreateInfo passInfo;
			passInfo.Name = "DirShadowMapPass";
			passInfo.Width = m_ShadowMapResolution;
			passInfo.Height = m_ShadowMapResolution;
			passInfo.Layers = ShaderDef::SHADOW_CASCADES_COUNT;
			passInfo.DebugColor = { 0.7f, 0.8f, 0.7f, 1.f };

			RenderTarget output = Texture2D::Create(shadowMapInfo);
			output.LoadOp = RenderTargetLoadOp::CLEAR;
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
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("DirShadowMap_Anim");
			pipelineInfo.VertexLayout = AnimVertex::GetLayout();

			m_DirShadowMapAnimPipeline = Pipeline::Create(pipelineInfo);
			m_DirShadowMapAnimPipeline->SetInput("u_ShadowsData", m_ShadowsUBO);
			m_DirShadowMapAnimPipeline->SetInput("u_BonesData", m_BonesSBO);
			m_DirShadowMapAnimPipeline->Bake();

			TextureSamplerCreateInfo samplerInfo;
			samplerInfo.MinFilter = TextureFilter::LINEAR;
			samplerInfo.MagFilter = TextureFilter::LINEAR;
			samplerInfo.Wrap = TextureWrap::CLAMP_TO_BORDER;
			samplerInfo.Compare = TextureCompareOperator::LEQUAL;
			m_ShadowMapSampler = Texture2D::Create(output.Texture->GetImage(), samplerInfo);
		}

		// GEOMETRY PASS
		{
			RenderPassCreateInfo passInfo;
			passInfo.Name = "GeometryPass";
			passInfo.InputPass = m_DirShadowMapPass;
			passInfo.Width = m_ViewportSize.x;
			passInfo.Height = m_ViewportSize.y;
			passInfo.DebugColor = { 0.4f, 0.8f, 0.2f, 1.f };

			m_GeometryPass = RenderPass::Create(passInfo);
			m_GeometryPass->SetOutput({ "SceneHDRColor", ImageFormat::RGBA16F });
			m_GeometryPass->SetOutput({ "SceneDepth", ImageFormat::DEPTH32F });
			m_GeometryPass->Bake();


			PipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "StaticGeometryPipeline";
			pipelineInfo.RenderPass = m_GeometryPass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("PBR_Static");
			pipelineInfo.VertexLayout = StaticVertex::GetLayout();
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompare = DepthCompare::LESS;
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
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("PBR_Anim");
			pipelineInfo.VertexLayout = AnimVertex::GetLayout();

			m_AnimGeometryPipeline = Pipeline::Create(pipelineInfo);

			m_AnimGeometryPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_AnimGeometryPipeline->SetInput("u_LightData", m_LightSBO);
			m_AnimGeometryPipeline->SetInput("u_RendererData", m_RendererUBO);
			m_AnimGeometryPipeline->SetInput("u_BonesData", m_BonesSBO);
			m_AnimGeometryPipeline->SetInput("u_ShadowsData", m_ShadowsUBO);
			m_AnimGeometryPipeline->SetInput("u_DirShadowMap", m_DirShadowMapPass->GetOutput("DirShadowMap"));
			m_AnimGeometryPipeline->SetInput("u_DirShadowMapShadow", m_ShadowMapSampler);
			m_AnimGeometryPipeline->SetInput("u_BRDF_LUT", Renderer::GetBRDF_LUT());
			m_AnimGeometryPipeline->SetInput("u_EnvironmentMap", Renderer::GetBlackTextureCube());
			m_AnimGeometryPipeline->SetInput("u_IrradianceMap", Renderer::GetBlackTextureCube());
			m_AnimGeometryPipeline->Bake();


			pipelineInfo.Name = "SkyboxPipeline";
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("Skybox");
			pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
			pipelineInfo.VertexLayout = { { ShaderDataType::Float3, "a_Position" } };
			pipelineInfo.BlendEnable = false;

			m_SkyboxPipeline = Pipeline::Create(pipelineInfo);
			m_SkyboxPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_SkyboxPipeline->SetInput("u_RendererData", m_RendererUBO);
			m_SkyboxPipeline->SetInput("u_EnvironmentMap", Renderer::GetBlackTextureCube());
			m_SkyboxPipeline->Bake();
		}

		// BLOOM PASS
		{
			Texture2DCreateInfo texInfo;
			texInfo.Name = "BloomTexture";
			texInfo.Format = ImageFormat::R11G11B10F;
			texInfo.Usage = ImageUsage(ImageUsage::STORAGE | ImageUsage::SAMPLED);
			texInfo.Width = 1;
			texInfo.Height = 1;
			texInfo.Layers = 1;
			texInfo.GenerateMipLevels = true;
			texInfo.SamplerInfo.MinFilter = TextureFilter::LINEAR;
			texInfo.SamplerInfo.MagFilter = TextureFilter::LINEAR;
			texInfo.SamplerInfo.MipMapFilter = TextureFilter::LINEAR;
			texInfo.SamplerInfo.Wrap = TextureWrap::CLAMP_TO_EDGE;

			m_BloomTexture = Texture2D::Create(texInfo);

			ComputePassCreateInfo passInfo;
			passInfo.Name = "BloomPass";
			passInfo.InputRenderPass = m_GeometryPass;
			passInfo.DebugColor = { 1.f, 0.05f, 0.55f, 1.f };

			m_BloomPass = ComputePass::Create(passInfo);
			m_BloomPass->SetOutput(m_BloomTexture);
			m_BloomPass->Bake();

			ComputePipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "BloomDownsample";
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("BloomDownsample");
			
			m_BloomDownsample = ComputePipeline::Create(pipelineInfo);
			m_BloomDownsample->SetInput("u_BloomTexture", m_BloomTexture);
			m_BloomDownsample->SetInput("u_SceneHDRColor", m_GeometryPass->GetOutput("SceneHDRColor"));
			m_BloomDownsample->Bake();
			
			pipelineInfo.Name = "BloomUpsample";
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("BloomUpsample");
			
			m_BloomUpsample = ComputePipeline::Create(pipelineInfo);
			m_BloomUpsample->SetInput("u_BloomTexture", m_BloomTexture);
			m_BloomUpsample->SetInput("u_DirtTexture", Renderer::GetBlackTexture());
			m_BloomUpsample->Bake();
		}

		// COMPOSITE PASS
		{
			Texture2DCreateInfo texInfo;
			texInfo.Name = "SceneColor";
			texInfo.Format = ImageFormat::RGBA8;
			texInfo.Usage = ImageUsage(ImageUsage::ATTACHMENT | ImageUsage::SAMPLED | ImageUsage::TRANSFER_SRC);
			texInfo.SamplerInfo.Wrap = TextureWrap::CLAMP_TO_EDGE;

			RenderPassCreateInfo passInfo;
			passInfo.Name = "SceneCompositePass";
			passInfo.InputPass = m_GeometryPass;
			passInfo.Width = m_ViewportSize.x;
			passInfo.Height = m_ViewportSize.y;
			passInfo.DebugColor = { 0.2f, 0.5f, 1.0f, 1.f };

			RenderTarget target = Texture2D::Create(texInfo);
			target.LoadOp = RenderTargetLoadOp::DONT_CARE;

			m_CompositePass = RenderPass::Create(passInfo);
			m_CompositePass->SetOutput(target);
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

			m_CompositePipeline->SetInput("u_SceneHDRColor", m_GeometryPass->GetOutput("SceneHDRColor"));
			m_CompositePipeline->SetInput("u_BloomTexture", m_BloomPass->GetOutput("BloomTexture"));
			m_CompositePipeline->Bake();

			m_CompositeMaterial = Material::Create(pipelineInfo.Shader, pipelineInfo.Name);
		}

		// Renderer2D Pass
		{
			RenderPassCreateInfo passInfo;
			passInfo.Name = "Renderer2DPass";
			passInfo.InputPass = m_CompositePass;
			passInfo.Width = m_ViewportSize.x;
			passInfo.Height = m_ViewportSize.y;
			passInfo.DebugColor = { 0.9f, 0.1f, 0.2f, 1.f };

			RenderTarget colorOutput = m_CompositePass->GetOutput("SceneColor");
			RenderTarget depthOutput = m_GeometryPass->GetOutput("SceneDepth");

			m_Render2DPass = RenderPass::Create(passInfo);
			m_Render2DPass->SetOutput(colorOutput);
			m_Render2DPass->SetOutput(depthOutput);
			m_Render2DPass->Bake();
		}

		// FXAA Compute Pass
		{
			Texture2DCreateInfo texInfo;
			texInfo.Name = "PostProcessTex";
			texInfo.Format = ImageFormat::RGBA8;
			texInfo.Usage = ImageUsage(ImageUsage::SAMPLED | ImageUsage::STORAGE | ImageUsage::TRANSFER_SRC);
			texInfo.SamplerInfo.Wrap = TextureWrap::CLAMP_TO_EDGE;

			m_PostProcessTexture = Texture2D::Create(texInfo);

			ComputePassCreateInfo passInfo;
			passInfo.Name = "FXAAPass";
			passInfo.InputRenderPass = m_Render2DPass;
			passInfo.DebugColor = { 0.75f, 0.1f, 0.8f, 1.f };

			m_FXAAPass = ComputePass::Create(passInfo);
			m_FXAAPass->SetOutput(m_PostProcessTexture);
			m_FXAAPass->Bake();

			ComputePipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "FXAAPipeline";
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("FXAA");

			m_FXAAPipeline = ComputePipeline::Create(pipelineInfo);
			m_FXAAPipeline->SetInput("u_SceneColor", m_Render2DPass->GetOutput("SceneColor"));
			m_FXAAPipeline->SetInput("u_PostProcessTex", m_PostProcessTexture);
			m_FXAAPipeline->SetInput("u_RendererData", m_RendererUBO);
			m_FXAAPipeline->Bake();
		}
	}

	void SceneRenderer::Shutdown()
	{
		
	}

	Ref<Texture2D> SceneRenderer::GetFinalImage()
	{
		if(m_Settings.DebugView == DebugView::DEPTH)
			return m_Render2DPass->GetOutput("SceneDepth");

		if (m_Settings.PostProcessingSettings.AntialisingMethod == Antialising::FXAA)
			return m_FXAAPass->GetOutput("PostProcessTex");
		
		return m_Render2DPass->GetOutput("SceneColor");
	}

	Ref<Texture2D> SceneRenderer::GetShadowMap()
	{
		return m_DirShadowMapPass->GetDepthOutput();
	}

	void SceneRenderer::OnViewportResize(uint32 width, uint32 height)
	{
		m_ViewportSize = { width, height };

		m_GeometryPass->Resize(width, height);
		m_StaticGeometryPipeline->SetViewport(width, height);
		m_AnimGeometryPipeline->SetViewport(width, height);
		m_SkyboxPipeline->SetViewport(width, height);

		m_BloomTexture->Resize(width, height);
		m_BloomMaterials.clear();

		m_CompositePass->Resize(width, height);
		m_CompositePipeline->SetViewport(width, height);

		m_Render2DPass->Resize(width, height);

		m_PostProcessTexture->Resize(width, height);
	}

	void SceneRenderer::OnRender2D(const Render2DCallback& callback)
	{
		m_Render2DCallback = callback;
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

		m_LightData.SpotLightCount = lightEnv.SpotLights.size();
		if (m_LightData.SpotLightCount > ShaderDef::MAX_SPOT_LIGHT_COUNT)
		{
			ATN_CORE_WARN_TAG("Renderer", "Attempt to submit more than {} SpotLights!", ShaderDef::MAX_SPOT_LIGHT_COUNT);
			m_LightData.SpotLightCount = ShaderDef::MAX_SPOT_LIGHT_COUNT;
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
		{
			m_LightData.PointLights[i] = lightEnv.PointLights[i];
		}

		for (uint32 i = 0; i < m_LightData.SpotLightCount; ++i)
		{
			m_LightData.SpotLights[i] = lightEnv.SpotLights[i];
			m_LightData.SpotLights[i].Direction.Normalize();
			m_LightData.SpotLights[i].SpotAngle = Math::Cos(Math::Radians(m_LightData.SpotLights[i].SpotAngle / 2.0));
		}

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

		m_ShadowsData.MaxDistance = m_Settings.ShadowSettings.MaxDistance;
		m_ShadowsData.FadeOut = m_Settings.ShadowSettings.FadeOut;
		m_ShadowsData.CascadeBlendDistance = m_Settings.ShadowSettings.CascadeBlendDistance;
		m_ShadowsData.SoftShadows = m_Settings.ShadowSettings.SoftShadows;

		if (m_Settings.BloomSettings.DirtTexture)
			m_BloomUpsample->SetInput("u_DirtTexture", m_Settings.BloomSettings.DirtTexture);
		else
			m_BloomUpsample->SetInput("u_DirtTexture", Renderer::GetBlackTexture());

		m_CompositeMaterial->Set("u_Mode", (uint32)m_Settings.PostProcessingSettings.TonemapMode);
		m_CompositeMaterial->Set("u_Exposure", m_Settings.PostProcessingSettings.Exposure);
		m_CompositeMaterial->Set("u_EnableBloom", (uint32)m_Settings.BloomSettings.Enable);

		m_RendererData.ViewportSize = m_ViewportSize;
		m_RendererData.InverseViewportSize = Vector2(1.f) / m_RendererData.ViewportSize;
		m_RendererData.DebugShadowCascades = m_Settings.DebugView == DebugView::SHADOW_CASCADES ? 1 : 0;

		m_BonesDataOffset = 0;
	}

	void SceneRenderer::EndScene()
	{
		ATN_PROFILE_FUNC();

		m_Profiler->Reset();
		m_Profiler->BeginPipelineStatsQuery();

		m_StaticGeometryList.Sort();
		m_AnimGeometryList.Sort();

		m_CameraUBO->UploadData(&m_CameraData, sizeof(CameraData));
		m_RendererUBO->UploadData(&m_RendererData, sizeof(RendererData));
		m_LightSBO->UploadData(&m_LightData, sizeof(LightData));
		m_ShadowsUBO->UploadData(&m_ShadowsData, sizeof(ShadowsData));
		m_BonesSBO->UploadData(m_BonesData.data(), m_BonesDataOffset * sizeof(Matrix4));

		DirShadowMapPass();
		GeometryPass();
		BloomPass();
		SceneCompositePass();
		Render2DPass();
		FXAAPass();

		m_Statistics.PipelineStats = m_Profiler->EndPipelineStatsQuery();
		m_Statistics.GPUTime = m_Statistics.DirShadowMapPass + m_Statistics.GeometryPass + 
			m_Statistics.BloomPass + m_Statistics.SceneCompositePass + m_Statistics.Render2DPass;

		m_StaticGeometryList.Clear();
		m_AnimGeometryList.Clear();
	}

	void SceneRenderer::DirShadowMapPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		m_DirShadowMapPass->Begin(commandBuffer);

		Renderer::BeginDebugRegion(commandBuffer, "StaticGeometry", { 0.8f, 0.4f, 0.2f, 1.f });
		{
			m_DirShadowMapStaticPipeline->Bind(commandBuffer);
			m_StaticGeometryList.FlushShadowPass(m_DirShadowMapStaticPipeline);
		}
		Renderer::EndDebugRegion(commandBuffer);


		Renderer::BeginDebugRegion(commandBuffer, "AnimatedGeometry", { 0.8f, 0.4f, 0.8f, 1.f });
		{
			m_DirShadowMapAnimPipeline->Bind(commandBuffer);
			m_AnimGeometryList.FlushShadowPass(m_DirShadowMapAnimPipeline);
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
		{
			m_StaticGeometryPipeline->Bind(commandBuffer);
			m_StaticGeometryList.Flush(m_StaticGeometryPipeline);
		}
		Renderer::EndDebugRegion(commandBuffer);
			

		Renderer::BeginDebugRegion(commandBuffer, "AnimatedGeometry", { 0.8f, 0.4f, 0.8f, 1.f });
		{
			m_AnimGeometryPipeline->Bind(commandBuffer);
			m_AnimGeometryList.Flush(m_AnimGeometryPipeline);
		}
		Renderer::EndDebugRegion(commandBuffer);


		Renderer::BeginDebugRegion(commandBuffer, "Skybox", { 0.3f, 0.6f, 0.6f, 1.f });
		{
			m_SkyboxPipeline->Bind(commandBuffer);
			Renderer::RenderNDCCube(commandBuffer, m_SkyboxPipeline);
		}
		Renderer::EndDebugRegion(commandBuffer);


		m_GeometryPass->End(commandBuffer);
		m_Statistics.GeometryPass = m_Profiler->EndTimeQuery();
	}

	void SceneRenderer::BloomPass()
	{
		if (!m_Settings.BloomSettings.Enable)
			return;

		auto commandBuffer = m_RenderCommandBuffer;

		// Calculate number of downsample passes
		const uint32 downSampleResLimit = 8;

		uint32 width = m_BloomTexture->GetWidth();
		uint32 height = m_BloomTexture->GetHeight();
		uint32 mipLevels = 1;

		while (width >= downSampleResLimit && height >= downSampleResLimit)
		{
			width /= 2;
			height /= 2;

			mipLevels++;
		}

		// Init materials
		m_BloomMaterials.resize(mipLevels);
		for(uint32 mip = 0; mip < mipLevels; ++mip)
		{
			Ref<Material>& material = m_BloomMaterials[mip];

			// Recreate (on window resized)
			if (material == nullptr)
			{
				material = Material::Create(Renderer::GetShaderPack()->Get("BloomDownsample"), std::format("BloomMaterial_{}", mip));
				material->Set("u_BloomTextureMip", m_BloomTexture, 0, mip);
			}

			material->Set("u_Intensity", m_Settings.BloomSettings.Intensity);
			material->Set("u_Threshold", m_Settings.BloomSettings.Threshold);
			material->Set("u_Knee", m_Settings.BloomSettings.Knee);
			material->Set("u_DirtIntensity", m_Settings.BloomSettings.DirtIntensity);
		}

		m_Profiler->BeginTimeQuery();
		m_BloomPass->Begin(commandBuffer);
		{
			// Read from mip - 1 and write to mip
			m_BloomDownsample->Bind(commandBuffer);
			for (uint32 mip = 1; mip < mipLevels; ++mip)
			{
				Ref<Material> material = m_BloomMaterials[mip];

				Vector2u mipSize = m_BloomTexture->GetMipSize(mip);
				material->Set("u_TexelSize", Vector2(1.f, 1.f) / Vector2(mipSize));
				material->Set("u_ReadMipLevel", mip - 1);

				material->Bind(commandBuffer);

				Renderer::Dispatch(commandBuffer, m_BloomDownsample, { mipSize.x, mipSize.y, 1 }, material);
				Renderer::InsertMemoryBarrier(commandBuffer);
			}

			// Read from mip and write to mip - 1
			m_BloomUpsample->Bind(commandBuffer);
			for (uint32 mip = mipLevels - 1; mip > 0; --mip)
			{
				Ref<Material> material = m_BloomMaterials[mip - 1];

				Vector2u mipSize = m_BloomTexture->GetMipSize(mip - 1);
				material->Set("u_TexelSize", Vector2(1.f, 1.f) / Vector2(mipSize));
				material->Set("u_ReadMipLevel", mip);
				material->Bind(commandBuffer);

				Renderer::Dispatch(commandBuffer, m_BloomUpsample, { mipSize.x, mipSize.y, 1 }, material);

				if (mip != 1)
					Renderer::InsertMemoryBarrier(commandBuffer);
			}
		}
		m_BloomPass->End(commandBuffer);
		m_Statistics.BloomPass = m_Profiler->EndTimeQuery();
	}

	void SceneRenderer::SceneCompositePass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		m_CompositePass->Begin(commandBuffer);
		{
			m_CompositePipeline->Bind(commandBuffer);
			Renderer::RenderFullscreenQuad(commandBuffer, m_CompositePipeline, m_CompositeMaterial);
		}
		m_CompositePass->End(commandBuffer);
		m_Statistics.SceneCompositePass = m_Profiler->EndTimeQuery();
	}

	void SceneRenderer::Render2DPass()
	{
		if (!m_Render2DCallback)
			return;

		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		m_Render2DPass->Begin(commandBuffer);
		{
			m_Render2DCallback();
		}
		m_Render2DPass->End(commandBuffer);
		m_Statistics.Render2DPass = m_Profiler->EndTimeQuery();
	}

	void SceneRenderer::FXAAPass()
	{
		if (m_Settings.PostProcessingSettings.AntialisingMethod != Antialising::FXAA)
			return;

		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		m_FXAAPass->Begin(commandBuffer);
		{
			m_FXAAPipeline->Bind(commandBuffer);
			Renderer::Dispatch(commandBuffer, m_FXAAPipeline, { m_ViewportSize.x, m_ViewportSize.y, 1 });
		}
		m_FXAAPass->End(commandBuffer);
		m_Statistics.FXAAPass = m_Profiler->EndTimeQuery();
	}

	void SceneRenderer::CalculateCascadeLightSpaces(const DirectionalLight& light)
	{
		float cameraNear = m_CameraData.NearClip;
		float cameraFar = m_CameraData.FarClip;

		float splitWeight = m_Settings.ShadowSettings.CascadeSplit;
		float cascadeNear = 0.f;

		// Split cascades
		for (uint32 i = 0; i < ShaderDef::SHADOW_CASCADES_COUNT; ++i)
		{
			float percent = (i + 1) / float(ShaderDef::SHADOW_CASCADES_COUNT);

			float log = cameraNear * Math::Pow(cameraFar / cameraNear, percent);
			float uniform = Math::Lerp(cameraNear, cameraFar, percent);
			float cascadeFar = Math::Lerp(uniform, log, splitWeight);

			m_ShadowsData.CascadePlanes[i].x = cascadeNear;
			m_ShadowsData.CascadePlanes[i].y = cascadeFar;

			cascadeNear = cascadeFar;
		}

		Matrix4 invCamera = Math::Inverse(m_CameraData.View * m_CameraData.Projection);
		float lastSplit = 0.f;

		// Build light space matrices
		for (uint32 layer = 0; layer < ShaderDef::SHADOW_CASCADES_COUNT; ++layer)
		{
			float cascadeFar = m_ShadowsData.CascadePlanes[layer].y;
			// convert to range (0, 1]
			float split = (cascadeFar - cameraNear) / (cameraFar - cameraNear);

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

			lastSplit = split;
		}
	}
}
