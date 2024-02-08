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
		m_RenderCommandBuffer = Renderer::GetRenderCommandBuffer();

		GPUProfilerCreateInfo profilerInfo;
		profilerInfo.Name = "SceneRendererProfiler";
		profilerInfo.RenderCommandBuffer = m_RenderCommandBuffer;
		profilerInfo.MaxTimestampsCount = 16;
		profilerInfo.MaxPipelineQueriesCount = 1;
		m_Profiler = GPUProfiler::Create(profilerInfo);

		m_CameraUBO = UniformBuffer::Create("CameraUBO", sizeof(CameraData));
		m_SceneUBO = UniformBuffer::Create("SceneUBO", sizeof(SceneData));
		m_LightSBO = StorageBuffer::Create("LightSBO", sizeof(LightData));

		// Geometry Pass
		{
			FramebufferCreateInfo fbInfo;
			fbInfo.Name = "GeometryFramebuffer";
			fbInfo.Attachments = { ImageFormat::RGBA16F, ImageFormat::DEPTH24STENCIL8 };
			fbInfo.Width = m_ViewportSize.x;
			fbInfo.Height = m_ViewportSize.y;
			fbInfo.Attachments[0].Name = "GeometryHDRColor";
			fbInfo.Attachments[0].ClearColor = { 0.9f, 0.3f, 0.4f, 1.0f };
			fbInfo.Attachments[1].Name = "GeometryDepth";
			fbInfo.Attachments[1].DepthClearColor = 1.f;
			fbInfo.Attachments[1].StencilClearColor = 1.f;

			RenderPassCreateInfo renderPassInfo;
			renderPassInfo.Name = "GeometryPass";
			renderPassInfo.Output = Framebuffer::Create(fbInfo);
			renderPassInfo.LoadOp = RenderPassLoadOp::CLEAR;

			m_GeometryPass = RenderPass::Create(renderPassInfo);


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
			m_StaticGeometryPipeline->Bake();


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
			FramebufferCreateInfo fbInfo;
			fbInfo.Name = "SceneCompositeFramebuffer";
			fbInfo.Attachments = { ImageFormat::RGBA8 };
			fbInfo.Width = m_ViewportSize.x;
			fbInfo.Height = m_ViewportSize.y;
			fbInfo.Attachments[0].Name = "SceneCompositeColor";

			RenderPassCreateInfo renderPassInfo;
			renderPassInfo.Name = "SceneCompositePass";
			renderPassInfo.Output = Framebuffer::Create(fbInfo);
			renderPassInfo.LoadOp = RenderPassLoadOp::DONT_CARE;

			m_CompositePass = RenderPass::Create(renderPassInfo);

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
	}

	void SceneRenderer::Shutdown()
	{
		
	}

	Ref<Image> SceneRenderer::GetFinalImage()
	{
		return m_CompositePass->GetOutput(0)->GetImage();
	}

	void SceneRenderer::OnViewportResize(uint32 width, uint32 height)
	{
		m_ViewportSize = { width, height };

		m_GeometryPass->GetOutput()->Resize(width, height);
		m_StaticGeometryPipeline->SetViewport(width, height);
		m_SkyboxPipeline->SetViewport(width, height);

		m_CompositePass->GetOutput()->Resize(width, height);
		m_CompositePipeline->SetViewport(width, height);
	}

	void SceneRenderer::BeginScene(const CameraInfo& cameraInfo)
	{
		m_CameraData.View = cameraInfo.ViewMatrix;
		m_CameraData.Projection = cameraInfo.ProjectionMatrix;
		m_CameraData.RotationView = cameraInfo.ViewMatrix.AsMatrix3();
		m_CameraData.Position = Math::AffineInverse(cameraInfo.ViewMatrix)[3];

		m_SceneData.Exposure = m_Settings.LightEnvironmentSettings.Exposure;
		m_SceneData.Gamma = m_Settings.LightEnvironmentSettings.Gamma;
	}

	void SceneRenderer::EndScene()
	{
		m_Profiler->Reset();
		m_Profiler->BeginPipelineStatsQuery();

		Renderer::Submit([this]()
		{
			m_CameraUBO->RT_SetData(&m_CameraData, sizeof(CameraData));
			m_SceneUBO->RT_SetData(&m_SceneData, sizeof(SceneData));
			m_LightSBO->RT_SetData(&m_LightData, sizeof(LightData));
		});

		GeometryPass();
		SceneCompositePass();

		m_Statistics.PipelineStats = m_Profiler->EndPipelineStatsQuery();
		m_StaticGeometryList.clear();
	}

	void SceneRenderer::Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform)
	{
		if (!vertexBuffer)
		{
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit null vertexBuffer!");
			return;
		}

		DrawCall drawCall;
		drawCall.VertexBuffer = vertexBuffer;
		drawCall.Transform = transform;
		drawCall.Material = material;

		m_StaticGeometryList.push_back(drawCall);
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

		for(uint32 i = 0; i < m_LightData.DirectionalLightCount; ++i)
			m_LightData.DirectionalLights[i] = lightEnv.DirectionalLights[i];

		for (uint32 i = 0; i < m_LightData.PointLightCount; ++i)
			m_LightData.PointLights[i] = lightEnv.PointLights[i];

		m_SceneData.EnvironmentIntensity = lightEnv.EnvironmentMapIntensity;
		m_SceneData.EnvironmentLOD = lightEnv.EnvironmentMapLOD;

		if(lightEnv.EnvironmentMap)
			m_SkyboxPipeline->SetInput("u_EnvironmentMap", lightEnv.EnvironmentMap->GetPrefilteredMap());
		else
			m_SkyboxPipeline->SetInput("u_EnvironmentMap", Renderer::GetBlackTextureCube());
	}

	void SceneRenderer::GeometryPass()
	{
		m_Profiler->BeginTimeQuery();
		auto commandBuffer = m_RenderCommandBuffer;

		m_GeometryPass->Begin(commandBuffer);
		{
			m_StaticGeometryPipeline->Bind(commandBuffer);
			
			for (const auto& drawCall : m_StaticGeometryList)
			{
				drawCall.Material->Bind(commandBuffer);
				drawCall.Material->Set("u_Transform", drawCall.Transform);
				Renderer::RenderGeometry(commandBuffer, drawCall.VertexBuffer, drawCall.Material);
			}

			m_SkyboxPipeline->Bind(commandBuffer);
			Renderer::RenderNDCCube(commandBuffer);
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
			Renderer::RenderFullscreenQuad(commandBuffer);
		}
		m_CompositePass->End(commandBuffer);

		m_Statistics.SceneCompositePass = m_Profiler->EndTimeQuery();
	}
}
