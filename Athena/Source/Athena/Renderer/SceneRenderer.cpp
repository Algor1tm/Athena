#include "SceneRenderer.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"
#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/DescriptorSetManager.h"


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
		const uint32 maxTimestamps = 16;
		const uint32 maxPipelineQueries = 1;
		m_Profiler = GPUProfiler::Create(maxTimestamps, maxPipelineQueries);

		m_CameraUBO = UniformBuffer::Create("CameraUBO", sizeof(CameraData));

		// Geometry Pass
		{
			FramebufferCreateInfo fbInfo;
			fbInfo.Name = "GeometryFramebuffer";
			fbInfo.Attachments = { TextureFormat::RGBA8, TextureFormat::DEPTH24STENCIL8 };
			fbInfo.Width = m_ViewportSize.x;
			fbInfo.Height = m_ViewportSize.y;
			fbInfo.Attachments[0].Name = "GeometryColor";
			fbInfo.Attachments[0].ClearColor = { 0.9f, 0.3f, 0.4f, 1.0f };
			fbInfo.Attachments[1].Name = "GeometryDepth";
			fbInfo.Attachments[1].DepthClearColor = 1.f;
			fbInfo.Attachments[1].StencilClearColor = 1.f;

			Ref<Framebuffer> mainFramebuffer = Framebuffer::Create(fbInfo);

			RenderPassCreateInfo renderPassInfo;
			renderPassInfo.Name = "GeometryPass";
			renderPassInfo.Output = mainFramebuffer;
			renderPassInfo.LoadOpClear = true;

			m_GeometryPass = RenderPass::Create(renderPassInfo);

			PipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "StaticGeometryPipeline";
			pipelineInfo.RenderPass = m_GeometryPass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("Test");
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompare = DepthCompare::LESS_OR_EQUAL;
			pipelineInfo.BlendEnable = true;

			m_StaticGeometryPipeline = Pipeline::Create(pipelineInfo);
			m_StaticGeometryPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_StaticGeometryPipeline->Bake();

			m_StaticGeometryMaterial = Material::Create(pipelineInfo.Shader, "StaticGeometryMaterial");
			m_StaticGeometryMaterial->Set("u_AlbedoMap", Renderer::GetWhiteTexture());
		}
	}

	void SceneRenderer::Shutdown()
	{
		
	}

	Ref<Texture2D> SceneRenderer::GetFinalImage()
	{
		return m_GeometryPass->GetOutput(0);
	}

	void SceneRenderer::OnViewportResize(uint32 width, uint32 height)
	{
		m_ViewportSize = { width, height };

		m_GeometryPass->GetOutput()->Resize(width, height);
		m_StaticGeometryPipeline->SetViewport(width, height);
	}

	void SceneRenderer::BeginScene(const CameraInfo& cameraInfo)
	{
		m_CameraData.View = cameraInfo.ViewMatrix;
		m_CameraData.Projection = cameraInfo.ProjectionMatrix;

		Renderer::Submit([this]()
		{
			m_CameraUBO->RT_SetData(&m_CameraData, sizeof(CameraData));
		});
	}

	void SceneRenderer::EndScene()
	{
		m_Profiler->Reset();
		m_Profiler->BeginPipelineStatsQuery();

		GeometryPass();

		m_Statistics.PipelineStats = m_Profiler->EndPipelineStatsQuery();

		m_StaticGeometryList.clear();
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
		drawCall.Material = material;

		m_StaticGeometryList.push_back(drawCall);
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
		m_Profiler->BeginTimeQuery();

		m_GeometryPass->Begin();
		{
			m_StaticGeometryPipeline->Bind();

			for (const auto& drawCall : m_StaticGeometryList)
			{
				drawCall.Material->Bind();
				drawCall.Material->Set("u_Transform", drawCall.Transform);
				Renderer::RenderMeshWithMaterial(drawCall.VertexBuffer, drawCall.Material);
			}
		}
		m_GeometryPass->End();

		m_Statistics.GeometryPass = m_Profiler->EndTimeQuery();
	}
}
