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

	void SceneRenderer::Init()
	{
		m_Data = new SceneRendererData();

		const uint32 maxTimestamps = 16;
		const uint32 maxPipelineQueries = 1;
		m_Data->Profiler = GPUProfiler::Create(maxTimestamps, maxPipelineQueries);

		m_Data->CameraUBO = UniformBuffer::Create(sizeof(CameraData));

		// Geometry Pass
		{
			FramebufferCreateInfo fbInfo;
			fbInfo.Attachments = { TextureFormat::RGBA8, TextureFormat::DEPTH24STENCIL8 };
			fbInfo.Width = m_Data->ViewportSize.x;
			fbInfo.Height = m_Data->ViewportSize.y;
			fbInfo.Attachments[0].ClearColor = { 0.9f, 0.3f, 0.4f, 1.0f };
			fbInfo.Attachments[1].DepthClearColor = 1.f;
			fbInfo.Attachments[1].StencilClearColor = 1.f;

			Ref<Framebuffer> mainFramebuffer = Framebuffer::Create(fbInfo);

			RenderPassCreateInfo renderPassInfo;
			renderPassInfo.Output = mainFramebuffer;
			renderPassInfo.LoadOpClear = true;

			m_Data->GeometryPass = RenderPass::Create(renderPassInfo);

			PipelineCreateInfo pipelineInfo;
			pipelineInfo.RenderPass = m_Data->GeometryPass;
			pipelineInfo.Material = Material::Create(Renderer::GetShaderPack()->Get("Test"));
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
			pipelineInfo.BlendEnable = true;

			m_Data->StaticGeometryPipeline = Pipeline::Create(pipelineInfo);

			pipelineInfo.Material->Set("u_CameraData", m_Data->CameraUBO);
		}
	}

	void SceneRenderer::Shutdown()
	{
		delete m_Data;
	}

	Ref<Texture2D> SceneRenderer::GetFinalImage()
	{
		return m_Data->GeometryPass->GetOutput(0);
	}

	void SceneRenderer::OnViewportResize(uint32 width, uint32 height)
	{
		m_Data->ViewportSize = { width, height };

		m_Data->GeometryPass->GetOutput()->Resize(width, height);
		m_Data->StaticGeometryPipeline->SetViewport(width, height);
	}

	void SceneRenderer::BeginScene(const CameraInfo& cameraInfo)
	{
		m_Data->CameraData.View = cameraInfo.ViewMatrix;
		m_Data->CameraData.Projection = cameraInfo.ProjectionMatrix;

		Renderer::Submit([this]()
		{
			m_Data->CameraUBO->RT_SetData(&m_Data->CameraData, sizeof(CameraData));
		});
	}

	void SceneRenderer::EndScene()
	{
		m_Data->Profiler->Reset();
		m_Data->Profiler->BeginPipelineStatsQuery();

		GeometryPass();

		m_Data->Statistics.PipelineStats = m_Data->Profiler->EndPipelineStatsQuery();

		m_Data->StaticGeometryList.clear();
	}

	void SceneRenderer::Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform)
	{
		if (!vertexBuffer)
		{
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit nullptr vertexBuffer!");
			return;
		}

		DrawCall drawCall;
		drawCall.VertexBuffer = vertexBuffer;
		drawCall.Transform = transform;

		m_Data->StaticGeometryList.push_back(drawCall);
	}

	void SceneRenderer::SubmitLightEnvironment(const LightEnvironment& lightEnv)
	{
		if (lightEnv.DirectionalLights.size() > ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT)
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit more than {} DirectionalLights!", ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT);

		if (lightEnv.PointLights.size() > ShaderDef::MAX_POINT_LIGHT_COUNT)
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit more than {} PointLights!", ShaderDef::MAX_POINT_LIGHT_COUNT);
	}

	void SceneRenderer::GeometryPass()
	{
		m_Data->Profiler->BeginTimeQuery();

		m_Data->GeometryPass->Begin();
		{
			m_Data->StaticGeometryPipeline->Bind();
			Ref<Material> material = m_Data->StaticGeometryPipeline->GetInfo().Material;

			for (const auto& drawCall : m_Data->StaticGeometryList)
			{
				material->Set("u_Transform", drawCall.Transform);
				Renderer::RenderMeshWithMaterial(drawCall.VertexBuffer, material);

			}
		}
		m_Data->GeometryPass->End();

		m_Data->Statistics.GeometryPass = m_Data->Profiler->EndTimeQuery();
	}
}
