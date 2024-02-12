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

		m_CameraUBO = UniformBuffer::Create("CameraUBO", sizeof(CameraData));
		m_SceneUBO = UniformBuffer::Create("SceneUBO", sizeof(SceneData));
		m_LightSBO = StorageBuffer::Create("LightSBO", sizeof(LightData));

		uint64 maxNumBones = ShaderDef::MAX_NUM_BONES_PER_MESH * ShaderDef::MAX_NUM_ANIMATED_MESHES;
		m_BonesSBO = StorageBuffer::Create("BonesSBO", maxNumBones * sizeof(Matrix4));

		m_BonesData = std::vector<Matrix4>(maxNumBones);
		m_BonesDataOffset = 0;

		// Geometry Pass
		{
			RenderPassCreateInfo passInfo;
			passInfo.Name = "GeometryPass";
			passInfo.Attachments = { ImageFormat::RGBA16F, ImageFormat::DEPTH24STENCIL8 };
			passInfo.Width = m_ViewportSize.x;
			passInfo.Height = m_ViewportSize.y;
			passInfo.Attachments[0].Name = "GeometryHDRColor";
			passInfo.Attachments[0].LoadOp = AttachmentLoadOp::CLEAR;
			passInfo.Attachments[0].ClearColor = { 0.f, 0.f, 0.f, 1.0f };
			passInfo.Attachments[1].Name = "GeometryDepth";
			passInfo.Attachments[1].LoadOp = AttachmentLoadOp::CLEAR;
			passInfo.Attachments[1].DepthClearColor = 1.f;
			passInfo.Attachments[1].StencilClearColor = 1.f;
			passInfo.DebugColor = { 0.4f, 0.8f, 0.2f, 1.f };

			m_GeometryPass = RenderPass::Create(passInfo);

			PipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "StaticGeometryPipeline";
			pipelineInfo.RenderPass = m_GeometryPass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("PBR_Static");
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
			pipelineInfo.BlendEnable = true;

			m_StaticGeometryPipeline = Pipeline::Create(pipelineInfo);

			m_StaticGeometryPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_StaticGeometryPipeline->SetInput("u_LightData", m_LightSBO);
			m_StaticGeometryPipeline->SetInput("u_SceneData", m_SceneUBO);
			m_StaticGeometryPipeline->SetInput("u_BRDF_LUT", Renderer::GetBRDF_LUT());
			m_StaticGeometryPipeline->SetInput("u_EnvironmentMap", Renderer::GetBlackTextureCube());
			m_StaticGeometryPipeline->SetInput("u_IrradianceMap", Renderer::GetBlackTextureCube());
			m_StaticGeometryPipeline->Bake();


			pipelineInfo.Name = "AnimGeometryPipeline";
			pipelineInfo.RenderPass = m_GeometryPass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("PBR_Anim");
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
			pipelineInfo.BlendEnable = true;

			m_AnimGeometryPipeline = Pipeline::Create(pipelineInfo);

			m_AnimGeometryPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_AnimGeometryPipeline->SetInput("u_LightData", m_LightSBO);
			m_AnimGeometryPipeline->SetInput("u_SceneData", m_SceneUBO);
			m_AnimGeometryPipeline->SetInput("u_BonesData", m_BonesSBO);
			m_AnimGeometryPipeline->SetInput("u_BRDF_LUT", Renderer::GetBRDF_LUT());
			m_AnimGeometryPipeline->SetInput("u_EnvironmentMap", Renderer::GetBlackTextureCube());
			m_AnimGeometryPipeline->SetInput("u_IrradianceMap", Renderer::GetBlackTextureCube());
			m_AnimGeometryPipeline->Bake();


			pipelineInfo.Name = "SkyboxPipeline";
			pipelineInfo.RenderPass = m_GeometryPass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("Skybox");
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
			pipelineInfo.BlendEnable = false;

			m_SkyboxPipeline = Pipeline::Create(pipelineInfo);
			m_SkyboxPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_SkyboxPipeline->SetInput("u_SceneData", m_SceneUBO);
			m_SkyboxPipeline->SetInput("u_EnvironmentMap", Renderer::GetBlackTextureCube());
			m_SkyboxPipeline->Bake();
		}

		// Composite Pass
		{
			RenderPassCreateInfo passInfo;
			passInfo.Name = "SceneCompositePass";
			passInfo.InputPass = m_GeometryPass;
			passInfo.Attachments = { ImageFormat::RGBA8 };
			passInfo.Width = m_ViewportSize.x;
			passInfo.Height = m_ViewportSize.y;
			passInfo.Attachments[0].Name = "SceneCompositeColor";
			passInfo.Attachments[0].LoadOp = AttachmentLoadOp::DONT_CARE;
			passInfo.DebugColor = { 0.2f, 0.5f, 1.0f, 1.f };

			m_CompositePass = RenderPass::Create(passInfo);

			PipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "SceneCompositePipeline";
			pipelineInfo.RenderPass = m_CompositePass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("SceneComposite");
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompare = DepthCompare::NONE;
			pipelineInfo.BlendEnable = false;

			m_CompositePipeline = Pipeline::Create(pipelineInfo);

			m_CompositePipeline->SetInput("u_SceneHDRColor", m_GeometryPass->GetOutput(0));
			m_CompositePipeline->SetInput("u_SceneData", m_SceneUBO);
			m_CompositePipeline->Bake();
		}

		// Renderer2D Pass
		{
			RenderPassCreateInfo passInfo;
			passInfo.Name = "Renderer2DPass";
			passInfo.InputPass = m_CompositePass;
			passInfo.Width = m_ViewportSize.x;
			passInfo.Height = m_ViewportSize.y;
			passInfo.ExistingImages.push_back(m_CompositePass->GetOutput(0)->GetImage());
			passInfo.ExistingImages.push_back(m_GeometryPass->GetDepthOutput()->GetImage());
			passInfo.DebugColor = { 0.9f, 0.1f, 0.2f, 1.f };

			m_Renderer2DPass = RenderPass::Create(passInfo);

			m_SceneRenderer2D = SceneRenderer2D::Create(m_Renderer2DPass);
		}
	}

	void SceneRenderer::Shutdown()
	{
		m_SceneRenderer2D.Release();
	}

	Ref<Image> SceneRenderer::GetFinalImage()
	{
		return m_CompositePass->GetOutput(0)->GetImage();
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
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit more than {} DirectionalLights!", ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT);
			m_LightData.DirectionalLightCount = ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT;
		}

		m_LightData.PointLightCount = lightEnv.PointLights.size();
		if (m_LightData.PointLightCount > ShaderDef::MAX_POINT_LIGHT_COUNT)
		{
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit more than {} PointLights!", ShaderDef::MAX_POINT_LIGHT_COUNT);
			m_LightData.PointLightCount = ShaderDef::MAX_POINT_LIGHT_COUNT;
		}

		for (uint32 i = 0; i < m_LightData.DirectionalLightCount; ++i)
			m_LightData.DirectionalLights[i] = lightEnv.DirectionalLights[i];

		for (uint32 i = 0; i < m_LightData.PointLightCount; ++i)
			m_LightData.PointLights[i] = lightEnv.PointLights[i];

		m_SceneData.EnvironmentIntensity = lightEnv.EnvironmentMapIntensity;
		m_SceneData.EnvironmentLOD = lightEnv.EnvironmentMapLOD;

		if (lightEnv.EnvironmentMap)
		{
			auto irradianceMap = lightEnv.EnvironmentMap->GetIrradianceMap();
			auto environmentMap = lightEnv.EnvironmentMap->GetPrefilteredMap();

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
		m_CameraData.RotationView = cameraInfo.ViewMatrix.AsMatrix3();
		m_CameraData.Position = Math::AffineInverse(cameraInfo.ViewMatrix)[3];

		m_SceneData.Exposure = m_Settings.LightEnvironmentSettings.Exposure;
		m_SceneData.Gamma = m_Settings.LightEnvironmentSettings.Gamma;

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
			m_SceneUBO->RT_SetData(&m_SceneData, sizeof(SceneData));
			m_LightSBO->RT_SetData(&m_LightData, sizeof(LightData));
			m_BonesSBO->RT_SetData(m_BonesData.data(), m_BonesDataOffset * sizeof(Matrix4));
		});

		GeometryPass();
		SceneCompositePass();

		m_Statistics.PipelineStats = m_Profiler->EndPipelineStatsQuery();

		m_StaticGeometryList.Clear();
		m_AnimGeometryList.Clear();
	}

	void SceneRenderer::GeometryPass()
	{
		m_Profiler->BeginTimeQuery();
		auto commandBuffer = m_RenderCommandBuffer;

		m_GeometryPass->Begin(commandBuffer);
		{
			Renderer::BeginDebugRegion(commandBuffer, "StaticGeometry", { 0.8f, 0.4f, 0.2f, 1.f });
			m_StaticGeometryPipeline->Bind(commandBuffer);

			for (const auto& drawCall : m_StaticGeometryList)
			{
				if(m_StaticGeometryList.UpdateMaterial(drawCall))
					drawCall.Material->Bind(commandBuffer);

				drawCall.Material->Set("u_Transform", drawCall.Transform);
				Renderer::RenderGeometry(commandBuffer, m_StaticGeometryPipeline, drawCall.VertexBuffer, drawCall.Material);
			}
			Renderer::EndDebugRegion(commandBuffer);
			

			Renderer::BeginDebugRegion(commandBuffer, "AnimatedGeometry", { 0.8f, 0.4f, 0.8f, 1.f });
			m_AnimGeometryPipeline->Bind(commandBuffer);

			for (const auto& drawCall : m_AnimGeometryList)
			{
				if (m_AnimGeometryList.UpdateMaterial(drawCall))
					drawCall.Material->Bind(commandBuffer);

				drawCall.Material->Set("u_Transform", drawCall.Transform);
				drawCall.Material->Set("u_BonesOffset", drawCall.BonesOffset);
				Renderer::RenderGeometry(commandBuffer, m_AnimGeometryPipeline, drawCall.VertexBuffer, drawCall.Material);
			}
			Renderer::EndDebugRegion(commandBuffer);


			Renderer::BeginDebugRegion(commandBuffer, "Skybox", { 0.3f, 0.6f, 0.6f, 1.f });
			{
				m_SkyboxPipeline->Bind(commandBuffer);
				Renderer::RenderNDCCube(commandBuffer, m_SkyboxPipeline);
			}
			Renderer::EndDebugRegion(commandBuffer);
		}
		m_GeometryPass->End(commandBuffer);

		m_Statistics.GeometryPass = m_Profiler->EndTimeQuery();
	}

	void SceneRenderer::SceneCompositePass()
	{
		m_Profiler->BeginTimeQuery();
		auto commandBuffer = m_RenderCommandBuffer;

		m_CompositePass->Begin(commandBuffer);
		{
			m_CompositePipeline->Bind(commandBuffer);
			Renderer::RenderFullscreenQuad(commandBuffer, m_CompositePipeline);
		}
		m_CompositePass->End(commandBuffer);

		m_Statistics.SceneCompositePass = m_Profiler->EndTimeQuery();
	}
}
