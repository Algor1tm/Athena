#include "SceneRenderer.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"

#include "Athena/Renderer/Renderer.h"


// TEMPORARY
#include "Athena/Core/Application.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"
#include "Athena/Platform/Vulkan/VulkanMaterial.h"
#include "Athena/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanRenderPass.h"
#include "Athena/Platform/Vulkan/VulkanFramebuffer.h"
#include <vulkan/vulkan.h>


namespace Athena
{
	static Ref<VertexBuffer> s_VertexBuffer;

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

		m_Data->CameraUBO = UniformBuffer::Create(sizeof(CameraUBO));

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


		{
			struct TVertex
			{
				Vector2 Position;
				LinearColor Color;
			};

			const TVertex vertices[] = {
				{ { -0.5f, -0.5f }, { 1.f, 0.f, 0.f, 1.f } },
				{ {  0.5f, -0.5f }, { 0.f, 1.f, 0.f, 0.f } },
				{ {  0.5f,  0.5f }, { 0.f, 1.f, 0.f, 1.f } },
				{ { -0.5f,  0.5f }, { 1.f, 1.f, 1.f, 1.f } }
			};

			const uint32 indices[] = {
				0, 1, 2, 2, 3, 0
			};

			VertexBufferCreateInfo info;
			info.VerticesData = (void*)vertices;
			info.VerticesSize = sizeof(vertices);
			info.IndicesData = (void*)indices;
			info.IndicesCount = std::size(indices);
			info.Usage = VertexBufferUsage::STATIC;

			s_VertexBuffer = VertexBuffer::Create(info);
		}
	}

	void SceneRenderer::Shutdown()
	{
		s_VertexBuffer.Release();
		delete m_Data;
	}

	// TEMPORARY
	void SceneRenderer::Render(const CameraInfo& cameraInfo)
	{
		m_Data->Profiler->Reset();

		m_Data->Profiler->BeginPipelineStatsQuery();
		m_Data->Profiler->BeginTimeQuery();

		m_Data->CameraData.ViewMatrix = cameraInfo.ViewMatrix;
		m_Data->CameraData.ProjectionMatrix = cameraInfo.ProjectionMatrix;

		// Update Uniform buffer
		Renderer::Submit([this]()
		{
			m_Data->CameraUBO->RT_SetData(&m_Data->CameraData, sizeof(CameraUBO));
		});

		m_Data->GeometryPass->Begin();
		{
			m_Data->StaticGeometryPipeline->Bind();

			Ref<Material> material = m_Data->StaticGeometryPipeline->GetInfo().Material;
			material->Set("u_Transform", Math::TranslateMatrix(Vector3{1, 0, 0}));

			Renderer::RenderMeshWithMaterial(s_VertexBuffer, material);
		}
		m_Data->GeometryPass->End();

		m_Data->Statistics.GeometryPass = m_Data->Profiler->EndTimeQuery();
		m_Data->Statistics.PipelineStats = m_Data->Profiler->EndPipelineStatsQuery();
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

	}

	void SceneRenderer::EndScene()
	{

	}

	void SceneRenderer::Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform, int32 entityID)
	{
		if (vertexBuffer)
		{
			//DrawCallInfo info;
			//info.VertexBuffer = vertexBuffer;
			//info.Material = material;
			//info.Animator = animator;
			//info.Transform = transform;
			//info.EntityID = entityID;
			//
			//s_Data.MeshList.Push(info);
		}
		else
		{
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit nullptr vertexBuffer!");
		}
	}

	void SceneRenderer::SubmitLightEnvironment(const LightEnvironment& lightEnv)
	{
		if (lightEnv.DirectionalLights.size() > ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT)
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit more than {} DirectionalLights!", ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT);

		if (lightEnv.PointLights.size() > ShaderDef::MAX_POINT_LIGHT_COUNT)
			ATN_CORE_WARN_TAG("SceneRenderer", "Attempt to submit more than {} PointLights!", ShaderDef::MAX_POINT_LIGHT_COUNT);

		//s_Data.LightDataBuffer.DirectionalLightCount = Math::Min(lightEnv.DirectionalLights.size(), (uint64)ShaderDef::MAX_DIRECTIONAL_LIGHT_COUNT);
		//for (uint32 i = 0; i < s_Data.LightDataBuffer.DirectionalLightCount; ++i)
		//{
		//	s_Data.LightDataBuffer.DirectionalLightBuffer[i] = lightEnv.DirectionalLights[i];
		//}
		//
		//s_Data.LightDataBuffer.PointLightCount = Math::Min(lightEnv.PointLights.size(), (uint64)ShaderDef::MAX_POINT_LIGHT_COUNT);
		//for (uint32 i = 0; i < s_Data.LightDataBuffer.PointLightCount; ++i)
		//{
		//	s_Data.LightDataBuffer.PointLightBuffer[i] = lightEnv.PointLights[i];
		//}
		//
		//s_Data.EnvironmentMap = lightEnv.EnvironmentMap;
		//
		//s_Data.EnvMapDataBuffer.LOD = lightEnv.EnvironmentMapLOD;
		//s_Data.EnvMapDataBuffer.Intensity = lightEnv.EnvironmentMapIntensity;
	}
}
