#include "SceneRenderer.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/TextureGenerator.h"


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
		profilerInfo.MaxTimestampsCount = 64;
		profilerInfo.MaxPipelineQueriesCount = 1;
		m_Profiler = GPUProfiler::Create(profilerInfo);

		m_CameraUBO = UniformBuffer::Create("CameraUBO", sizeof(CameraData));
		m_RendererUBO = UniformBuffer::Create("RendererUBO", sizeof(RendererData));
		m_ShadowsUBO = UniformBuffer::Create("ShadowsUBO", sizeof(ShadowsData));
		m_HBAO_UBO = UniformBuffer::Create("HBAO-UBO", sizeof(HBAOData));
		m_SSR_UBO = UniformBuffer::Create("SSR-UBO", sizeof(SSRData));

		m_BonesSBO = StorageBuffer::Create("BonesSBO", 1 * sizeof(Matrix4), BufferMemoryFlags::CPU_WRITEABLE);
		m_LightSBO = StorageBuffer::Create("LightSBO", sizeof(LightData), BufferMemoryFlags::CPU_WRITEABLE);
		m_VisibleLightsSBO = StorageBuffer::Create("VisibleLightsSBO", sizeof(TileVisibleLights) * 1, BufferMemoryFlags::GPU_ONLY);

		m_BonesDataOffset = 0;

		// For now instance rendering does not fully used, because we do not
		// reuse vertex buffers, (every vertex buffer has only 1 instance).
		// To fix that, we need some sort of Asset Manager to store 'MeshSource', to which
		// StaticMeshes would refer to.

		VertexMemoryLayout instanceLayout = {
				{ ShaderDataType::Float3, "a_TRow0" },
				{ ShaderDataType::Float3, "a_TRow1" },
				{ ShaderDataType::Float3, "a_TRow2" },
				{ ShaderDataType::Float3, "a_TRow2" } };

		VertexBufferCreateInfo instanceBufferInfo;
		instanceBufferInfo.Name = "TransformsStorage";
		instanceBufferInfo.Size = sizeof(InstanceTransformData) * 1;
		instanceBufferInfo.Flags = BufferMemoryFlags::CPU_WRITEABLE;

		m_TransformsStorage = VertexBuffer::Create(instanceBufferInfo);

		PipelineCreateInfo fullscreenPipeline;
		fullscreenPipeline.VertexLayout = {
			{ ShaderDataType::Float2, "a_Position" } };
		fullscreenPipeline.Topology = Topology::TRIANGLE_LIST;
		fullscreenPipeline.CullMode = CullMode::NONE;
		fullscreenPipeline.DepthTest = false;
		fullscreenPipeline.BlendEnable = false;

		// DIR SHADOW MAP PASS
		{
			for (uint32 i = 0; i < ShaderDef::SHADOW_CASCADES_COUNT; ++i)
				m_ShadowsData.DirLightViewProjection[i] = Matrix4::Identity();

			TextureCreateInfo shadowMapInfo;
			shadowMapInfo.Name = "DirShadowMap";
			shadowMapInfo.Format = TextureFormat::DEPTH32F;
			shadowMapInfo.Usage = TextureUsage(TextureUsage::ATTACHMENT | TextureUsage::SAMPLED);
			shadowMapInfo.Width = m_ShadowMapResolution;
			shadowMapInfo.Height = m_ShadowMapResolution;
			shadowMapInfo.Layers = ShaderDef::SHADOW_CASCADES_COUNT;
			shadowMapInfo.GenerateMipMap = false;
			shadowMapInfo.Sampler.Filter = TextureFilter::NEAREST;
			shadowMapInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_BORDER;

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
			pipelineInfo.InstanceLayout = instanceLayout;
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompareOp = DepthCompareOperator::LESS_OR_EQUAL;
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

			TextureViewCreateInfo viewInfo;
			viewInfo.LayerCount = ShaderDef::SHADOW_CASCADES_COUNT;
			viewInfo.OverrideSampler = true;
			viewInfo.Sampler.Filter = TextureFilter::LINEAR;
			viewInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_BORDER;
			viewInfo.Sampler.Compare = TextureCompareOperator::LESS_OR_EQUAL;

			m_ShadowMapSampler = output.Texture->GetView(viewInfo);
		}

		// GBUFFER PASS
		{
			RenderPassCreateInfo passInfo;
			passInfo.Name = "GBufferPass";
			passInfo.Width = m_ViewportSize.x;
			passInfo.Height = m_ViewportSize.y;
			passInfo.DebugColor = { 0.35f, 0.1f, 0.7f, 1.f };

			m_GBufferPass = RenderPass::Create(passInfo);
			// RGBA -> RGB - albedo, A - empty
			m_GBufferPass->SetOutput({ "SceneAlbedo", TextureFormat::RGBA8, TextureFilter::NEAREST });
			// RGBA -> RGB - normal, A - emission
			m_GBufferPass->SetOutput({ "SceneNormalsEmission", TextureFormat::RGBA16F, TextureFilter::NEAREST });
			// RG -> R - roughness, G - metalness
			m_GBufferPass->SetOutput({ "SceneRoughnessMetalness", TextureFormat::RG8, TextureFilter::NEAREST });
			m_GBufferPass->SetOutput({ "SceneDepth", TextureFormat::DEPTH32F, TextureFilter::NEAREST });
			m_GBufferPass->Bake();

			PipelineCreateInfo pipelineInfo;
			pipelineInfo.Name = "StaticGeometryPipeline";
			pipelineInfo.RenderPass = m_GBufferPass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("GBuffer_Static");
			pipelineInfo.VertexLayout = StaticVertex::GetLayout();
			pipelineInfo.InstanceLayout = instanceLayout;
			pipelineInfo.Topology = Topology::TRIANGLE_LIST;
			pipelineInfo.CullMode = CullMode::BACK;
			pipelineInfo.DepthCompareOp = DepthCompareOperator::GREATER;
			pipelineInfo.BlendEnable = false;

			m_StaticGeometryPipeline = Pipeline::Create(pipelineInfo);
			m_StaticGeometryPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_StaticGeometryPipeline->Bake();

			pipelineInfo.Name = "AnimGeometryPipeline";
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("GBuffer_Anim");
			pipelineInfo.VertexLayout = AnimVertex::GetLayout();

			m_AnimGeometryPipeline = Pipeline::Create(pipelineInfo);
			m_AnimGeometryPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_AnimGeometryPipeline->SetInput("u_BonesData", m_BonesSBO);
			m_AnimGeometryPipeline->Bake();
		}

		// Hi-Z
		{
			TextureCreateInfo texInfo;
			texInfo.Name = "HiZBuffer";
			texInfo.Format = TextureFormat::R32F;
			texInfo.Usage = TextureUsage(TextureUsage::STORAGE | TextureUsage::SAMPLED);
			texInfo.GenerateMipMap = true;
			texInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;
			texInfo.Sampler.Filter = TextureFilter::NEAREST;

			m_HiZBuffer = Texture2D::Create(texInfo);

			ComputePassCreateInfo passInfo;
			passInfo.Name = "HiZPass";
			passInfo.InputRenderPass = m_GBufferPass;
			passInfo.DebugColor = { 1.f, 1.f, 0.5f, 1.f };

			m_HiZPass = ComputePass::Create(passInfo);
			m_HiZPass->SetOutput(m_HiZBuffer);
			m_HiZPass->Bake();

			m_HiZPipeline = ComputePipeline::Create(Renderer::GetShaderPack()->Get("HiZ"));
			m_HiZPipeline->Bake();

			for (uint32 i = 0; i < ShaderDef::HIZ_MIP_LEVEL_COUNT; ++i)
			{
				Ref<Material> material = Material::Create(Renderer::GetShaderPack()->Get("HiZ"), fmt::format("HiZ_{}", i));

				if (i == 0)
				{
					material->Set("u_SourceDepth", m_GBufferPass->GetOutput("SceneDepth"));
					material->Set("u_OutputDepth", m_HiZBuffer->GetMipView(0));
				}
				else
				{
					material->Set("u_SourceDepth", m_HiZBuffer->GetMipView(i - 1));
					material->Set("u_OutputDepth", m_HiZBuffer->GetMipView(i));
				}

				m_HiZMaterials.push_back(material);
			}
		}

		// LIGHT CULLING COMPUTE PASS
		{
			ComputePassCreateInfo passInfo;
			passInfo.Name = "LightCullingPass";
			passInfo.InputRenderPass = m_GBufferPass;
			passInfo.DebugColor = { 0.7f, 0.7f, 0.7f, 1.f };

			m_LightCullingPass = ComputePass::Create(passInfo);
			m_LightCullingPass->SetOutput(m_VisibleLightsSBO);
			m_LightCullingPass->Bake();

			m_LightCullingPipeline = ComputePipeline::Create(Renderer::GetShaderPack()->Get("LightCulling"));
			m_LightCullingPipeline->SetInput("u_LightData", m_LightSBO);
			m_LightCullingPipeline->SetInput("u_VisibleLightsData", m_VisibleLightsSBO);
			m_LightCullingPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_LightCullingPipeline->SetInput("u_RendererData", m_RendererUBO);
			m_LightCullingPipeline->SetInput("u_SceneDepth", m_GBufferPass->GetOutput("SceneDepth"));
			m_LightCullingPipeline->Bake();
		}

		// HBAO
		{
			// Deinterleave
			{
				TextureCreateInfo texInfo;
				texInfo.Name = "HBAO-DepthLayers";
				texInfo.Format = TextureFormat::R32F;
				texInfo.Usage = TextureUsage(TextureUsage::SAMPLED | TextureUsage::STORAGE);
				texInfo.Layers = 16;
				texInfo.Sampler.Filter = TextureFilter::NEAREST;
				texInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

				Ref<Texture2D> hbaoLayers = Texture2D::Create(texInfo);

				ComputePassCreateInfo passInfo;
				passInfo.Name = "HBAO-Deinterleave";
				passInfo.InputRenderPass = m_GBufferPass;
				passInfo.DebugColor = { 0.3f, 0.6f, 0.6f, 1.f };

				m_HBAODeinterleavePass = ComputePass::Create(passInfo);
				m_HBAODeinterleavePass->SetOutput(hbaoLayers);
				m_HBAODeinterleavePass->Bake();

				m_HBAODeinterleavePipeline = ComputePipeline::Create(Renderer::GetShaderPack()->Get("HBAO-Deinterleave"));
				m_HBAODeinterleavePipeline->SetInput("u_DepthLayers", hbaoLayers);
				m_HBAODeinterleavePipeline->SetInput("u_CameraData", m_CameraUBO);
				m_HBAODeinterleavePipeline->SetInput("u_HBAOData", m_HBAO_UBO);
				m_HBAODeinterleavePipeline->SetInput("u_SceneDepth", m_GBufferPass->GetOutput("SceneDepth"));
				m_HBAODeinterleavePipeline->Bake();
			}

			// Compute
			{
				TextureCreateInfo texInfo;
				texInfo.Name = "HBAO-Output";
				texInfo.Format = TextureFormat::RG16F;
				texInfo.Usage = TextureUsage(TextureUsage::SAMPLED | TextureUsage::STORAGE);
				texInfo.Sampler.Filter = TextureFilter::LINEAR;
				texInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

				Ref<Texture2D> hbaoOutput = Texture2D::Create(texInfo);

				ComputePassCreateInfo passInfo;
				passInfo.Name = "HBAO-Compute";
				passInfo.InputComputePass = m_HBAODeinterleavePass;
				passInfo.DebugColor = { 0.3f, 0.6f, 0.6f, 1.f };

				m_HBAOComputePass = ComputePass::Create(passInfo);
				m_HBAOComputePass->SetOutput(hbaoOutput);
				m_HBAOComputePass->Bake();

				m_HBAOComputePipeline = ComputePipeline::Create(Renderer::GetShaderPack()->Get("HBAO-Compute"));
				m_HBAOComputePipeline->SetInput("u_Output", hbaoOutput);
				m_HBAOComputePipeline->SetInput("u_HBAOData", m_HBAO_UBO);
				m_HBAOComputePipeline->SetInput("u_DepthLayers", m_HBAODeinterleavePass->GetOutput("HBAO-DepthLayers"));
				m_HBAOComputePipeline->SetInput("u_SceneNormals", m_GBufferPass->GetOutput("SceneNormalsEmission"));
				m_HBAOComputePipeline->Bake();

				std::vector<Vector4> jitters = Noise::GetHBAOJitters(16);
				
				for (uint32 i = 0; i < 16; ++i)
				{
					m_HBAOData.Jitters[i] = jitters[i];
					m_HBAOData.Float2Offsets[i] = Vector4(float(i % 4) + 0.5f, float(i / 4) + 0.5f, 0.0f, 0.0f);
				}
			}

			// Blur X
			{
				RenderPassCreateInfo passInfo;
				passInfo.Name = "HBAO-BlurX";
				passInfo.DebugColor = { 0.3f, 0.6f, 0.6f, 1.f };

				RenderTarget blurTarget = { "HBAO-BlurredX", TextureFormat::RG16F, TextureFilter::LINEAR };
				blurTarget.ClearColor = Vector4(1.0);

				m_HBAOBlurXPass = RenderPass::Create(passInfo);
				m_HBAOBlurXPass->SetOutput(blurTarget);
				m_HBAOBlurXPass->Bake();

				PipelineCreateInfo pipelineInfo = fullscreenPipeline;
				pipelineInfo.Name = "HBAO-BlurXPipeline";
				pipelineInfo.RenderPass = m_HBAOBlurXPass;
				pipelineInfo.Shader = Renderer::GetShaderPack()->Get("HBAO-BlurX");

				m_HBAOBlurXPipeline = Pipeline::Create(pipelineInfo);
				m_HBAOBlurXPipeline->SetInput("u_HBAOData", m_HBAO_UBO);
				m_HBAOBlurXPipeline->SetInput("u_AODepth", m_HBAOComputePass->GetOutput("HBAO-Output"));
				m_HBAOBlurXPipeline->Bake();
			}

			// Blur Y
			{
				RenderPassCreateInfo passInfo;
				passInfo.Name = "HBAO-BlurY";
				passInfo.InputPass = m_HBAOBlurXPass;
				passInfo.DebugColor = { 0.3f, 0.6f, 0.6f, 1.f };

				RenderTarget blurTarget = { "SceneAO", TextureFormat::R8, TextureFilter::NEAREST };
				blurTarget.ClearColor = Vector4(1.0);

				m_HBAOBlurYPass = RenderPass::Create(passInfo);
				m_HBAOBlurYPass->SetOutput(blurTarget);
				m_HBAOBlurYPass->Bake();

				PipelineCreateInfo pipelineInfo = fullscreenPipeline;
				pipelineInfo.Name = "HBAO-BlurYPipeline";
				pipelineInfo.RenderPass = m_HBAOBlurYPass;
				pipelineInfo.Shader = Renderer::GetShaderPack()->Get("HBAO-BlurY");

				m_HBAOBlurYPipeline = Pipeline::Create(pipelineInfo);
				m_HBAOBlurYPipeline->SetInput("u_HBAOData", m_HBAO_UBO);
				m_HBAOBlurYPipeline->SetInput("u_AODepth", m_HBAOBlurXPass->GetOutput("HBAO-BlurredX"));
				m_HBAOBlurYPipeline->Bake();
			}
		}

		// DEFERRED LIGHTING PASS
		{
			TextureCreateInfo texInfo;
			texInfo.Name = "SceneHDRColor";
			texInfo.Format = TextureFormat::RGBA16F;
			texInfo.Usage = TextureUsage(TextureUsage::ATTACHMENT | TextureUsage::STORAGE | TextureUsage::SAMPLED);
			texInfo.Sampler.Filter = TextureFilter::LINEAR;
			texInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

			RenderPassCreateInfo passInfo;
			passInfo.Name = "DeferredLightingPass";
			passInfo.InputPass = m_HBAOBlurYPass;
			passInfo.Width = m_ViewportSize.x;
			passInfo.Height = m_ViewportSize.y;
			passInfo.DebugColor = { 0.4f, 0.8f, 0.2f, 1.f };

			RenderTarget target;
			target.Texture = Texture2D::Create(texInfo);
			target.LoadOp = RenderTargetLoadOp::CLEAR;

			m_DeferredLightingPass = RenderPass::Create(passInfo);
			m_DeferredLightingPass->SetOutput(target);
			m_DeferredLightingPass->Bake();

			PipelineCreateInfo pipelineInfo = fullscreenPipeline;
			pipelineInfo.Name = "DeferredLightingPipeline";
			pipelineInfo.RenderPass = m_DeferredLightingPass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("DeferredLighting");

			m_DeferredLightingPipeline = Pipeline::Create(pipelineInfo);

			m_DeferredLightingPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_DeferredLightingPipeline->SetInput("u_LightData", m_LightSBO);
			m_DeferredLightingPipeline->SetInput("u_VisibleLightsData", m_LightCullingPass->GetOutput("VisibleLightsSBO"));
			m_DeferredLightingPipeline->SetInput("u_RendererData", m_RendererUBO);
			m_DeferredLightingPipeline->SetInput("u_ShadowsData", m_ShadowsUBO);
			m_DeferredLightingPipeline->SetInput("u_DirShadowMap", m_DirShadowMapPass->GetOutput("DirShadowMap"));
			m_DeferredLightingPipeline->SetInput("u_DirShadowMapShadow", m_ShadowMapSampler);
			m_DeferredLightingPipeline->SetInput("u_PCSSNoise", TextureGenerator::GetBlueNoise());
			m_DeferredLightingPipeline->SetInput("u_BRDF_LUT", TextureGenerator::GetBRDF_LUT());
			m_DeferredLightingPipeline->SetInput("u_EnvironmentMap", TextureGenerator::GetBlackTextureCube());
			m_DeferredLightingPipeline->SetInput("u_IrradianceMap", TextureGenerator::GetBlackTextureCube());

			m_DeferredLightingPipeline->SetInput("u_SceneDepth", m_GBufferPass->GetOutput("SceneDepth"));
			m_DeferredLightingPipeline->SetInput("u_SceneAlbedo", m_GBufferPass->GetOutput("SceneAlbedo"));
			m_DeferredLightingPipeline->SetInput("u_SceneNormalsEmission", m_GBufferPass->GetOutput("SceneNormalsEmission"));
			m_DeferredLightingPipeline->SetInput("u_SceneRoughnessMetalness", m_GBufferPass->GetOutput("SceneRoughnessMetalness"));
			m_DeferredLightingPipeline->SetInput("u_SceneAO", m_HBAOBlurYPass->GetOutput("SceneAO"));

			m_DeferredLightingPipeline->Bake();
		}

		// SKYBOX PASS
		{
			RenderPassCreateInfo passInfo;
			passInfo.Name = "SkyboxPass";
			passInfo.InputPass = m_DeferredLightingPass;
			passInfo.Width = m_ViewportSize.x;
			passInfo.Height = m_ViewportSize.y;
			passInfo.DebugColor = { 0.4f, 0.8f, 0.2f, 1.f };

			m_SkyboxPass = RenderPass::Create(passInfo);
			m_SkyboxPass->SetOutput(m_DeferredLightingPass->GetOutput("SceneHDRColor"));
			m_SkyboxPass->SetOutput(m_GBufferPass->GetOutput("SceneDepth"));
			m_SkyboxPass->Bake();

			PipelineCreateInfo pipelineInfo = fullscreenPipeline;
			pipelineInfo.Name = "SkyboxPipeline";
			pipelineInfo.RenderPass = m_SkyboxPass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("Skybox");
			pipelineInfo.DepthTest = true;
			pipelineInfo.DepthWrite = true;
			pipelineInfo.DepthCompareOp = DepthCompareOperator::GREATER_OR_EQUAL;

			m_SkyboxPipeline = Pipeline::Create(pipelineInfo);
			m_SkyboxPipeline->SetInput("u_CameraData", m_CameraUBO);
			m_SkyboxPipeline->SetInput("u_RendererData", m_RendererUBO);
			m_SkyboxPipeline->SetInput("u_EnvironmentMap", TextureGenerator::GetBlackTextureCube());
			m_SkyboxPipeline->Bake();
		}

		// Pre-Convolution Pass
		{
			TextureCreateInfo texInfo;
			texInfo.Name = "HiColorBuffer";
			texInfo.Format = TextureFormat::R11G11B10F;
			texInfo.Usage = TextureUsage(TextureUsage::STORAGE | TextureUsage::SAMPLED);
			texInfo.GenerateMipMap = true;
			texInfo.Sampler.Filter = TextureFilter::LINEAR;
			texInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

			m_HiColorBuffer = Texture2D::Create(texInfo);

			texInfo.Name = "TmpBlurTexture";
			m_BlurTmpTexture = Texture2D::Create(texInfo);

			ComputePassCreateInfo passInfo;
			passInfo.Name = "Pre-ConvolutionPass";
			passInfo.InputRenderPass = m_SkyboxPass;
			passInfo.DebugColor = { 0.8f, 0.2f, 0.55f, 1.f };

			m_PreConvolutionPass = ComputePass::Create(passInfo);
			m_PreConvolutionPass->SetOutput(m_HiColorBuffer);
			m_PreConvolutionPass->SetOutput(m_BlurTmpTexture);
			m_PreConvolutionPass->Bake();

			m_PreConvolutionPipeline = ComputePipeline::Create(Renderer::GetShaderPack()->Get("Pre-Convolution"));
			m_PreConvolutionPipeline->Bake();

			for (uint32 i = 0; i < ShaderDef::PRECONVOLUTION_MIP_LEVEL_COUNT; ++i)
			{
				if (i == 0)
				{
					Ref<Material> material = Material::Create(Renderer::GetShaderPack()->Get("Pre-Convolution"), fmt::format("Pre-Convolution_{}", i));
					material->Set("u_SourceImage", m_DeferredLightingPass->GetOutput("SceneHDRColor"));
					material->Set("u_Mode", 1u); // Copy
					material->Set("u_OutputImage", m_HiColorBuffer->GetMipView(i));
					m_PreConvolutionMaterials.push_back(material);

				}
				else
				{
					Ref<Material> materialDown = Material::Create(Renderer::GetShaderPack()->Get("Pre-Convolution"), fmt::format("Pre-Convolution_Down{}", i));
					materialDown->Set("u_SourceImage", m_HiColorBuffer->GetMipView(i - 1));
					materialDown->Set("u_Mode", 4u); // downsample
					materialDown->Set("u_OutputImage", m_HiColorBuffer->GetMipView(i));
					m_PreConvolutionMaterials.push_back(materialDown);

					Ref<Material> materialBlurX = Material::Create(Renderer::GetShaderPack()->Get("Pre-Convolution"), fmt::format("Pre-Convolution_BlurX{}", i));
					materialBlurX->Set("u_SourceImage", m_HiColorBuffer->GetMipView(i));
					materialBlurX->Set("u_OutputImage", m_BlurTmpTexture->GetMipView(i - 1));
					materialBlurX->Set("u_Mode", 2u); // blur X
					m_PreConvolutionMaterials.push_back(materialBlurX);

					Ref<Material> materialBlurY = Material::Create(Renderer::GetShaderPack()->Get("Pre-Convolution"), fmt::format("Pre-Convolution_BlurY{}", i));
					materialBlurY->Set("u_SourceImage", m_BlurTmpTexture->GetMipView(i - 1));
					materialBlurY->Set("u_OutputImage", m_HiColorBuffer->GetMipView(i));
					materialBlurY->Set("u_Mode", 3u); // blur Y
					m_PreConvolutionMaterials.push_back(materialBlurY);
				}
			}
		}

		// SSR Pass
		{
			// SSR-Compute
			{
				TextureCreateInfo texInfo;
				texInfo.Name = "SSR-Output";
				texInfo.Format = TextureFormat::RGBA16F;
				texInfo.Usage = TextureUsage(TextureUsage::STORAGE | TextureUsage::SAMPLED);
				texInfo.Sampler.Filter = TextureFilter::NEAREST;
				texInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

				Ref<Texture2D> ssrTarget = Texture2D::Create(texInfo);

				ComputePassCreateInfo passInfo;
				passInfo.Name = "SSR-ComputePass";
				passInfo.InputComputePass = m_PreConvolutionPass;
				passInfo.DebugColor = { 0.2f, 0.5f, 1.0f, 1.f };

				m_SSRComputePass = ComputePass::Create(passInfo);
				m_SSRComputePass->SetOutput(ssrTarget);
				m_SSRComputePass->Bake();

				m_SSRComputePipeline = ComputePipeline::Create(Renderer::GetShaderPack()->Get("SSR-Compute"));
				m_SSRComputePipeline->SetInput("u_Output", ssrTarget);
				m_SSRComputePipeline->SetInput("u_HiZBuffer", m_HiZPass->GetOutput("HiZBuffer"));
				m_SSRComputePipeline->SetInput("u_SceneNormals", m_GBufferPass->GetOutput("SceneNormalsEmission"));
				m_SSRComputePipeline->SetInput("u_SceneRoughnessMetalness", m_GBufferPass->GetOutput("SceneRoughnessMetalness"));
				m_SSRComputePipeline->SetInput("u_SSRData", m_SSR_UBO);
				m_SSRComputePipeline->SetInput("u_CameraData", m_CameraUBO);
				m_SSRComputePipeline->Bake();
			}

			// SSR-Composite
			{
				ComputePassCreateInfo passInfo;
				passInfo.Name = "SSR-CompositePass";
				passInfo.InputComputePass = m_SSRComputePass;
				passInfo.DebugColor = { 0.2f, 0.5f, 1.0f, 1.f };

				m_SSRCompositePass = ComputePass::Create(passInfo);
				m_SSRCompositePass->SetOutput(m_DeferredLightingPass->GetOutput("SceneHDRColor"));
				m_SSRCompositePass->Bake();

				m_SSRCompositePipeline = ComputePipeline::Create(Renderer::GetShaderPack()->Get("SSR-Composite"));
				m_SSRCompositePipeline->SetInput("u_SceneColorOutput", m_DeferredLightingPass->GetOutput("SceneHDRColor"));
				m_SSRCompositePipeline->SetInput("u_HiZBuffer", m_HiZPass->GetOutput("HiZBuffer"));
				m_SSRCompositePipeline->SetInput("u_HiColorBuffer", m_PreConvolutionPass->GetOutput("HiColorBuffer"));
				m_SSRCompositePipeline->SetInput("u_SSROutput", m_SSRComputePass->GetOutput("SSR-Output"));
				m_SSRCompositePipeline->SetInput("u_SceneAlbedo", m_GBufferPass->GetOutput("SceneAlbedo"));
				m_SSRCompositePipeline->SetInput("u_SceneNormalsEmission", m_GBufferPass->GetOutput("SceneNormalsEmission"));
				m_SSRCompositePipeline->SetInput("u_SceneRoughnessMetalness", m_GBufferPass->GetOutput("SceneRoughnessMetalness"));
				m_SSRCompositePipeline->SetInput("u_SSRData", m_SSR_UBO);
				m_SSRCompositePipeline->SetInput("u_CameraData", m_CameraUBO);
				m_SSRCompositePipeline->Bake();
			}
		}

		// BLOOM PASS
		{
			ComputePassCreateInfo passInfo;
			passInfo.Name = "BloomPass";
			passInfo.InputComputePass = m_SSRCompositePass;
			passInfo.DebugColor = { 1.f, 0.05f, 0.55f, 1.f };

			m_BloomPass = ComputePass::Create(passInfo);
			m_BloomPass->SetOutput(m_HiColorBuffer);
			m_BloomPass->Bake();

			m_BloomDownsample = ComputePipeline::Create(Renderer::GetShaderPack()->Get("BloomDownsample"));
			m_BloomDownsample->SetInput("u_SceneHDRColor", m_SSRCompositePass->GetOutput("SceneHDRColor"));
			m_BloomDownsample->SetInput("u_BloomTexture", m_HiColorBuffer);
			m_BloomDownsample->Bake();

			m_BloomUpsample = ComputePipeline::Create(Renderer::GetShaderPack()->Get("BloomUpsample"));
			m_BloomUpsample->SetInput("u_BloomTexture", m_HiColorBuffer);
			m_BloomUpsample->SetInput("u_DirtTexture", TextureGenerator::GetBlackTexture());
			m_BloomUpsample->Bake();
		}

		// SCENE COMPOSITE PASS
		{
			RenderPassCreateInfo passInfo;
			passInfo.Name = "SceneCompositePass";
			//passInfo.InputPass = m_SkyboxPass;
			passInfo.Width = m_ViewportSize.x;
			passInfo.Height = m_ViewportSize.y;
			passInfo.DebugColor = { 0.8f, 0.7f, 0.1f, 1.f };

			RenderTarget target = RenderTarget("SceneColor", TextureFormat::RGBA8);
			target.LoadOp = RenderTargetLoadOp::DONT_CARE;

			m_SceneCompositePass = RenderPass::Create(passInfo);
			m_SceneCompositePass->SetOutput(target);
			m_SceneCompositePass->Bake();

			PipelineCreateInfo pipelineInfo = fullscreenPipeline;
			pipelineInfo.Name = "SceneCompositePipeline";
			pipelineInfo.RenderPass = m_SceneCompositePass;
			pipelineInfo.Shader = Renderer::GetShaderPack()->Get("SceneComposite");

			m_SceneCompositePipeline = Pipeline::Create(pipelineInfo);

			m_SceneCompositePipeline->SetInput("u_SceneHDRColor", m_SkyboxPass->GetOutput("SceneHDRColor"));
			m_SceneCompositePipeline->SetInput("u_BloomTexture", m_BloomPass->GetOutput("HiColorBuffer"));
			m_SceneCompositePipeline->Bake();

			m_SceneCompositeMaterial = Material::Create(pipelineInfo.Shader, pipelineInfo.Name);
		}

		// JUMP FLOOD
		{
			// MESH FILL PASS
			{
				RenderPassCreateInfo passInfo;
				passInfo.Name = "JumpFloodSilhouettePass";
				passInfo.InputPass = m_SceneCompositePass;
				passInfo.DebugColor = { 0.9f, 0.5f, 0.3f, 1.f };

				m_JumpFloodSilhouettePass = RenderPass::Create(passInfo);
				m_JumpFloodSilhouettePass->SetOutput({ "JumpFloodSilhouette", TextureFormat::R8, TextureFilter::NEAREST });
				m_JumpFloodSilhouettePass->Bake();

				PipelineCreateInfo pipelineInfo;
				pipelineInfo.Name = "JFSilhouetteStaticPipeline";
				pipelineInfo.RenderPass = m_JumpFloodSilhouettePass;
				pipelineInfo.Shader = Renderer::GetShaderPack()->Get("JumpFlood-Silhouette_Static");
				pipelineInfo.VertexLayout = StaticVertex::GetLayout();
				pipelineInfo.InstanceLayout = instanceLayout;
				pipelineInfo.Topology = Topology::TRIANGLE_LIST;
				pipelineInfo.CullMode = CullMode::BACK;
				pipelineInfo.DepthTest = false;
				pipelineInfo.BlendEnable = false;

				m_JFSilhouetteStaticPipeline = Pipeline::Create(pipelineInfo);

				m_JFSilhouetteStaticPipeline->SetInput("u_CameraData", m_CameraUBO);
				m_JFSilhouetteStaticPipeline->Bake();

				pipelineInfo.Name = "JFSilhouetteAnimPipeline";
				pipelineInfo.Shader = Renderer::GetShaderPack()->Get("JumpFlood-Silhouette_Anim");
				pipelineInfo.VertexLayout = AnimVertex::GetLayout();

				m_JFSilhouetteAnimPipeline = Pipeline::Create(pipelineInfo);
				m_JFSilhouetteAnimPipeline->SetInput("u_CameraData", m_CameraUBO);
				m_JFSilhouetteAnimPipeline->SetInput("u_BonesData", m_BonesSBO);
				m_JFSilhouetteAnimPipeline->Bake();
			}

			// TEXTURES INIT
			Ref<Texture2D> jumpFloodTextures[2];
			for (uint32 i = 0; i < 2; ++i)
			{
				TextureCreateInfo texInfo;
				texInfo.Name = std::format("JumpFloodPingPong_{}", i);
				texInfo.Format = TextureFormat::RG16F;
				texInfo.Usage = TextureUsage(TextureUsage::SAMPLED | TextureUsage::ATTACHMENT);
				texInfo.Sampler.Filter = TextureFilter::NEAREST;
				texInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

				jumpFloodTextures[i] = Texture2D::Create(texInfo);
			}

			// JUMP FLOOD INIT
			{
				RenderPassCreateInfo passInfo;
				passInfo.Name = "JumpFloodInitPass";
				passInfo.InputPass = m_JumpFloodSilhouettePass;
				passInfo.DebugColor = { 0.9f, 0.5f, 0.3f, 1.f };

				m_JumpFloodInitPass = RenderPass::Create(passInfo);
				m_JumpFloodInitPass->SetOutput(jumpFloodTextures[0]);
				m_JumpFloodInitPass->Bake();

				PipelineCreateInfo pipelineInfo = fullscreenPipeline;
				pipelineInfo.Name = "JumpFloodInitPipeline";
				pipelineInfo.RenderPass = m_JumpFloodInitPass;
				pipelineInfo.Shader = Renderer::GetShaderPack()->Get("JumpFlood-Init");

				m_JumpFloodInitPipeline = Pipeline::Create(pipelineInfo);

				m_JumpFloodInitPipeline->SetInput("u_SilhouetteTexture", m_JumpFloodSilhouettePass->GetOutput("JumpFloodSilhouette"));
				m_JumpFloodInitPipeline->Bake();
			}

			// JUMP FLOOD PASS
			{
				for (uint32 i = 0; i < 2; ++i)
				{
					uint32 index = (i + 1) % 2;
					bool even = index % 2 == 0;
					std::string_view label = even ? "Even" : "Odd";

					RenderPassCreateInfo passInfo;
					passInfo.Name = std::format("JumpFloodPass{}", label);
					passInfo.InputPass = even ? m_JumpFloodPasses[1] : m_JumpFloodInitPass;
					passInfo.DebugColor = { 0.9f, 0.5f, 0.3f, 1.f };

					m_JumpFloodPasses[index] = RenderPass::Create(passInfo);
					m_JumpFloodPasses[index]->SetOutput(jumpFloodTextures[index]);
					m_JumpFloodPasses[index]->Bake();

					PipelineCreateInfo pipelineInfo = fullscreenPipeline;
					pipelineInfo.Name = std::format("JumpFloodPipeline", label);
					pipelineInfo.RenderPass = m_JumpFloodPasses[index];
					pipelineInfo.Shader = Renderer::GetShaderPack()->Get("JumpFlood-Pass");

					m_JumpFloodPipelines[index] = Pipeline::Create(pipelineInfo);;
					m_JumpFloodPipelines[index]->SetInput("u_Texture", jumpFloodTextures[even ? 1 : 0]);
					m_JumpFloodPipelines[index]->Bake();
				}

				m_JumpFloodMaterial = Material::Create(Renderer::GetShaderPack()->Get("JumpFlood-Pass"), "JumpFloodMaterial");
			}

			// JUMP FLOOD COMPOSITE
			{
				uint32 jumps = Math::Ceil(Math::Log2(m_OutlineWidth + 1.f)) - 1;
				uint32 index = jumps % 2 == 1 ? 0 : 1;

				RenderPassCreateInfo passInfo;
				passInfo.Name = "JumpFloodCompositePass";
				passInfo.InputPass = m_JumpFloodPasses[index];
				passInfo.DebugColor = { 0.9f, 0.5f, 0.3f, 1.f };

				m_JumpFloodCompositePass = RenderPass::Create(passInfo);
				m_JumpFloodCompositePass->SetOutput(m_SceneCompositePass->GetOutput("SceneColor"));
				m_JumpFloodCompositePass->Bake();

				PipelineCreateInfo pipelineInfo = fullscreenPipeline;
				pipelineInfo.Name = "JumpFloodCompositePipeline";
				pipelineInfo.RenderPass = m_JumpFloodCompositePass;
				pipelineInfo.Shader = Renderer::GetShaderPack()->Get("JumpFlood-Composite");
				pipelineInfo.BlendEnable = true;

				m_JumpFloodCompositePipeline = Pipeline::Create(pipelineInfo);
				m_JumpFloodCompositePipeline->SetInput("u_Texture", m_JumpFloodPasses[index]->GetOutput(std::format("JumpFloodPingPong_{}", index)));
				m_JumpFloodCompositePipeline->Bake();

				m_JumpFloodCompositeMaterial = Material::Create(pipelineInfo.Shader, "JumpFloodCompositeMaterial");

				m_JumpFloodCompositeMaterial->Set("u_OutlineColor", m_OutlineColor);
				m_JumpFloodCompositeMaterial->Set("u_OutlineWidth", m_OutlineWidth);
			}
		}

		// RENDERER 2D Pass
		{
			RenderPassCreateInfo passInfo;
			passInfo.Name = "Renderer2DPass";
			passInfo.InputPass = m_SceneCompositePass;
			passInfo.Width = m_ViewportSize.x;
			passInfo.Height = m_ViewportSize.y;
			passInfo.DebugColor = { 0.9f, 0.1f, 0.2f, 1.f };

			m_Render2DPass = RenderPass::Create(passInfo);
			m_Render2DPass->SetOutput(m_SceneCompositePass->GetOutput("SceneColor"));
			m_Render2DPass->SetOutput(m_GBufferPass->GetOutput("SceneDepth"));
			m_Render2DPass->Bake();
		}

		// Reusable post-process textures (ping - pong)
		{
			TextureCreateInfo texInfo;
			texInfo.Format = TextureFormat::RGBA8;
			texInfo.Usage = TextureUsage(TextureUsage::ATTACHMENT | TextureUsage::SAMPLED | TextureUsage::STORAGE);
			texInfo.Sampler.Filter = TextureFilter::LINEAR;
			texInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;
			texInfo.GenerateMipMap = false;

			for (uint32 i = 0; i < 2; ++i)
			{
				texInfo.Name = std::format("PostProcessTex_{}", i);
				m_PostProcessTextures[i] = Texture2D::Create(texInfo);
			}
		}

		// SMAA
		{
			// Edge detection
			{
				RenderPassCreateInfo passInfo;
				passInfo.Name = "SMAA-EdgesPass";
				passInfo.InputPass = m_Render2DPass;
				passInfo.DebugColor = { 0.75f, 0.1f, 0.8f, 1.f };

				RenderTarget smaaEdges = m_PostProcessTextures[0];
				smaaEdges.LoadOp = RenderTargetLoadOp::CLEAR;

				m_SMAAEdgesPass = RenderPass::Create(passInfo);
				m_SMAAEdgesPass->SetOutput(smaaEdges);
				m_SMAAEdgesPass->Bake();

				PipelineCreateInfo pipelineInfo = fullscreenPipeline;
				pipelineInfo.Name = "SMAA-EdgesPipeline";
				pipelineInfo.RenderPass = m_SMAAEdgesPass;
				pipelineInfo.Shader = Renderer::GetShaderPack()->Get("SMAA-Edges");

				m_SMAAEdgesPipeline = Pipeline::Create(pipelineInfo);
				m_SMAAEdgesPipeline->SetInput("u_Texture", m_Render2DPass->GetOutput("SceneColor"));
				m_SMAAEdgesPipeline->SetInput("u_RendererData", m_RendererUBO);
				m_SMAAEdgesPipeline->Bake();
			}
			
			// Blending Weight Calculation
			{
				RenderPassCreateInfo passInfo;
				passInfo.Name = "SMAA-WeightsPass";
				passInfo.InputPass = m_SMAAEdgesPass;
				passInfo.DebugColor = { 0.75f, 0.1f, 0.8f, 1.f };

				RenderTarget smaaWeights = m_PostProcessTextures[1];
				smaaWeights.LoadOp = RenderTargetLoadOp::CLEAR;

				m_SMAAWeightsPass = RenderPass::Create(passInfo);
				m_SMAAWeightsPass->SetOutput(smaaWeights);
				m_SMAAWeightsPass->Bake();

				PipelineCreateInfo pipelineInfo = fullscreenPipeline;
				pipelineInfo.Name = "SMAA-WeightsPipeline";
				pipelineInfo.RenderPass = m_SMAAWeightsPass;
				pipelineInfo.Shader = Renderer::GetShaderPack()->Get("SMAA-Weights");

				m_SMAAWeightsPipeline = Pipeline::Create(pipelineInfo);
				m_SMAAWeightsPipeline->SetInput("u_Edges", m_SMAAEdgesPass->GetOutput(0));
				m_SMAAWeightsPipeline->SetInput("u_AreaTex", TextureGenerator::GetSMAA_AreaLUT());
				m_SMAAWeightsPipeline->SetInput("u_SearchTex", TextureGenerator::GetSMAA_SearchLUT());
				m_SMAAWeightsPipeline->SetInput("u_RendererData", m_RendererUBO);
				m_SMAAWeightsPipeline->Bake();
			}

			// Neighborhood Blending
			{
				RenderPassCreateInfo passInfo;
				passInfo.Name = "SMAA-BlendingPass";
				passInfo.InputPass = m_SMAAWeightsPass;
				passInfo.DebugColor = { 0.75f, 0.1f, 0.8f, 1.f };

				RenderTarget smaaBlend = m_PostProcessTextures[0];
				smaaBlend.LoadOp = RenderTargetLoadOp::CLEAR;

				m_SMAABlendingPass = RenderPass::Create(passInfo);
				m_SMAABlendingPass->SetOutput(smaaBlend);
				m_SMAABlendingPass->Bake();

				PipelineCreateInfo pipelineInfo = fullscreenPipeline;
				pipelineInfo.Name = "SMAA-BlendingPipeline";
				pipelineInfo.RenderPass = m_SMAABlendingPass;
				pipelineInfo.Shader = Renderer::GetShaderPack()->Get("SMAA-Blending");

				m_SMAABlendingPipeline = Pipeline::Create(pipelineInfo);
				m_SMAABlendingPipeline->SetInput("u_SceneColor", m_Render2DPass->GetOutput(0));
				m_SMAABlendingPipeline->SetInput("u_BlendTex", m_SMAAWeightsPass->GetOutput(0));
				m_SMAABlendingPipeline->SetInput("u_RendererData", m_RendererUBO);
				m_SMAABlendingPipeline->Bake();
			}
		}

		// FXAA COMPUTE PASS
		{

			ComputePassCreateInfo passInfo;
			passInfo.Name = "FXAAPass";
			passInfo.InputRenderPass = m_Render2DPass;
			passInfo.DebugColor = { 0.75f, 0.1f, 0.8f, 1.f };

			m_FXAAPass = ComputePass::Create(passInfo);
			m_FXAAPass->SetOutput(m_PostProcessTextures[0]);
			m_FXAAPass->Bake();

			m_FXAAPipeline = ComputePipeline::Create(Renderer::GetShaderPack()->Get("FXAA"));
			m_FXAAPipeline->SetInput("u_SceneColor", m_Render2DPass->GetOutput("SceneColor"));
			m_FXAAPipeline->SetInput("u_Texture", m_PostProcessTextures[0]);
			m_FXAAPipeline->SetInput("u_RendererData", m_RendererUBO);
			m_FXAAPipeline->Bake();
		}
	}

	void SceneRenderer::Shutdown()
	{

	}

	Ref<Texture2D> SceneRenderer::GetFinalImage()
	{
		Antialising antialising = GetAntialising();

		if (antialising == Antialising::FXAA)
			return m_FXAAPass->GetOutput(0);

		else if (antialising == Antialising::SMAA)
			return m_SMAABlendingPass->GetOutput(0);

		return m_Render2DPass->GetOutput("SceneColor");
	}

	Ref<Texture2D> SceneRenderer::GetShadowMap()
	{
		return m_DirShadowMapPass->GetDepthOutput();
	}

	void SceneRenderer::OnViewportResize(uint32 width, uint32 height)
	{
		m_OriginalViewportSize = { width, height };

		width = m_Settings.Quality.RendererScale * width;
		height = m_Settings.Quality.RendererScale * height;

		uint32 halfWidth = (width + 1) / 2;
		uint32 halfHeight = (height + 1) / 2;

		uint32 quarterWidth = ((width + 3) / 4);
		uint32 quarterHeight = ((height + 3) / 4);

		m_ViewportSize = { width, height };

		m_RendererData.ViewportSize = m_ViewportSize;
		m_RendererData.InverseViewportSize = Vector2(1.f) / m_RendererData.ViewportSize;
		m_RendererData.ViewportTilesCount.x = (width - 1) / ShaderDef::LIGHT_TILE_SIZE + 1;
		m_RendererData.ViewportTilesCount.y = (height - 1) / ShaderDef::LIGHT_TILE_SIZE + 1;
		m_RendererData.ViewportTilesCount.z = m_RendererData.ViewportTilesCount.x * m_RendererData.ViewportTilesCount.y;
		m_RendererData.ViewportTilesCount.w = 0;

		m_HBAOData.InvResolution = m_RendererData.InverseViewportSize;
		m_HBAOData.InvQuarterResolution = Vector2(1.f) / Vector2(quarterWidth, quarterHeight);

		m_VisibleLightsSBO->Resize(sizeof(TileVisibleLights) * m_RendererData.ViewportTilesCount.z);

		m_GBufferPass->Resize(width, height);
		m_StaticGeometryPipeline->SetViewport(width, height);
		m_AnimGeometryPipeline->SetViewport(width, height);

		m_HiZBuffer->Resize(width, height);

		m_HBAODeinterleavePass->GetOutput(0).As<Texture2D>()->Resize(quarterWidth, quarterHeight);
		m_HBAOComputePass->GetOutput(0).As<Texture2D>()->Resize(width, height);
		m_HBAOBlurXPass->Resize(width, height);
		m_HBAOBlurXPipeline->SetViewport(width, height);
		m_HBAOBlurYPass->Resize(width, height);
		m_HBAOBlurYPipeline->SetViewport(width, height);

		m_DeferredLightingPass->Resize(width, height);
		m_DeferredLightingPipeline->SetViewport(width, height);

		m_SkyboxPass->Resize(width, height);
		m_SkyboxPipeline->SetViewport(width, height);

		m_HiColorBuffer->Resize(width, height);
		m_BlurTmpTexture->Resize(halfWidth, halfHeight);

		if (m_Settings.SSRSettings.HalfRes)
			m_SSRComputePass->GetOutput(0).As<Texture2D>()->Resize(halfWidth, halfHeight);
		else
			m_SSRComputePass->GetOutput(0).As<Texture2D>()->Resize(width, height);

		m_BloomMaterials.clear();

		m_SceneCompositePass->Resize(width, height);
		m_SceneCompositePipeline->SetViewport(width, height);

		m_JumpFloodSilhouettePass->Resize(width, height);
		m_JFSilhouetteStaticPipeline->SetViewport(width, height);
		m_JFSilhouetteAnimPipeline->SetViewport(width, height);
		m_JumpFloodInitPass->Resize(width, height);
		m_JumpFloodInitPipeline->SetViewport(width, height);
		for (uint32 i = 0; i < 2; ++i)
		{
			m_JumpFloodPasses[i]->Resize(width, height);
			m_JumpFloodPipelines[i]->SetViewport(width, height);
		}
		m_JumpFloodCompositePass->Resize(width, height);
		m_JumpFloodCompositePipeline->SetViewport(width, height);

		m_Render2DPass->Resize(width, height);

		m_PostProcessTextures[0]->Resize(width, height);
		m_PostProcessTextures[1]->Resize(width, height);

		m_SMAAEdgesPass->Resize(width, height);
		m_SMAAEdgesPipeline->SetViewport(width, height);
		m_SMAAWeightsPass->Resize(width, height);
		m_SMAAWeightsPipeline->SetViewport(width, height);
		m_SMAABlendingPass->Resize(width, height);
		m_SMAABlendingPipeline->SetViewport(width, height);

		if(m_ViewportResizeCallback)
			m_ViewportResizeCallback(width, height);
	}

	void SceneRenderer::SetOnRender2DCallback(const Render2DCallback& callback)
	{
		m_Render2DCallback = callback;
	}

	void SceneRenderer::SetOnViewportResizeCallback(const OnViewportResizeCallback& callback)
	{
		m_ViewportResizeCallback = callback;
	}

	void SceneRenderer::Submit(const Ref<StaticMesh>& mesh, const Matrix4& transform)
	{
		if (mesh->HasAnimations())
		{
			SubmitAnimMesh(m_AnimGeometryList, mesh, mesh->GetAnimator(), transform);
		}
		else
		{
			SubmitStaticMesh(m_StaticGeometryList, mesh, transform);
		}
	}

	void SceneRenderer::SubmitSelectionContext(const Ref<StaticMesh>& mesh, const Matrix4& transform)
	{
		if (mesh->HasAnimations())
		{
			SubmitAnimMesh(m_SelectAnimGeometryList, mesh, mesh->GetAnimator(), transform);
		}
		else
		{
			SubmitStaticMesh(m_SelectStaticGeometryList, mesh, transform);
		}
	}

	void SceneRenderer::SubmitStaticMesh(DrawListStatic& list, const Ref<StaticMesh>& mesh, const Matrix4& transform)
	{
		const auto& subMeshes = mesh->GetAllSubMeshes();
		for (uint32 i = 0; i < subMeshes.size(); ++i)
		{
			Ref<Material> material = subMeshes[i].Material;

			StaticDrawCall drawCall;
			drawCall.VertexBuffer = subMeshes[i].VertexBuffer;
			drawCall.Transform = transform;
			drawCall.Material = material;

			list.Push(drawCall);
		}
	}

	void SceneRenderer::SubmitAnimMesh(DrawListAnim& list, const Ref<StaticMesh>& mesh, const Ref<Animator>& animator, const Matrix4& transform)
	{
		const auto& subMeshes = mesh->GetAllSubMeshes();
		for (uint32 i = 0; i < subMeshes.size(); ++i)
		{
			Ref<Material> material = subMeshes[i].Material;

			AnimDrawCall drawCall;
			drawCall.VertexBuffer = subMeshes[i].VertexBuffer;
			drawCall.Transform = transform;
			drawCall.Material = material;
			drawCall.BonesOffset = m_BonesDataOffset;

			const auto& bones = animator->GetBoneTransforms();
			m_BonesSBO.Push(bones.data(), bones.size() * sizeof(Matrix4));

			m_BonesDataOffset += bones.size();

			list.Push(drawCall);
		}
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

			m_DeferredLightingPipeline->SetInput("u_IrradianceMap", irradianceMap);
			m_DeferredLightingPipeline->SetInput("u_EnvironmentMap", environmentMap);
			m_SkyboxPipeline->SetInput("u_EnvironmentMap", environmentMap);
		}
		else
		{
			m_DeferredLightingPipeline->SetInput("u_IrradianceMap", TextureGenerator::GetBlackTextureCube());
			m_DeferredLightingPipeline->SetInput("u_EnvironmentMap", TextureGenerator::GetBlackTextureCube());
			m_SkyboxPipeline->SetInput("u_EnvironmentMap", TextureGenerator::GetBlackTextureCube());
		}
	}

	void SceneRenderer::BeginScene(const CameraInfo& cameraInfo)
	{
		m_RenderCommandBuffer = Renderer::GetRenderCommandBuffer();

		m_CameraData.View = cameraInfo.ViewMatrix;
		m_CameraData.InverseView = Math::Inverse(cameraInfo.ViewMatrix);
		m_CameraData.Projection = cameraInfo.ProjectionMatrix;
		m_CameraData.InverseProjection = Math::Inverse(cameraInfo.ProjectionMatrix);
		m_CameraData.ViewProjection = cameraInfo.ViewMatrix * cameraInfo.ProjectionMatrix;
		m_CameraData.InverseViewProjection = Math::Inverse(Matrix4(cameraInfo.ViewMatrix.AsMatrix3()) * cameraInfo.ProjectionMatrix); // no translation
		m_CameraData.Position = m_CameraData.InverseView[3];
		m_CameraData.NearClip = cameraInfo.NearClip;
		m_CameraData.FarClip = cameraInfo.FarClip;
		m_CameraData.FOV = cameraInfo.FOV;

		float projX = m_CameraData.InverseProjection[0][0];
		float projY = m_CameraData.InverseProjection[1][1];
		m_CameraData.ProjInfo = Vector4(2.0, 2.0, -1.0, -1.0) * Vector4(projX, projY, projX, projY);

		m_ShadowsData.MaxDistance = m_Settings.ShadowSettings.MaxDistance;
		m_ShadowsData.FadeOut = m_Settings.ShadowSettings.FadeOut;
		m_ShadowsData.CascadeBlendDistance = m_Settings.ShadowSettings.CascadeBlendDistance;
		m_ShadowsData.BiasGradient = m_Settings.ShadowSettings.BiasGradient;
		m_ShadowsData.SoftShadows = m_Settings.ShadowSettings.SoftShadows;

		if (m_Settings.BloomSettings.DirtTexture)
			m_BloomUpsample->SetInput("u_DirtTexture", m_Settings.BloomSettings.DirtTexture);
		else
			m_BloomUpsample->SetInput("u_DirtTexture", TextureGenerator::GetBlackTexture());

		m_SceneCompositeMaterial->Set("u_Mode", (uint32)m_Settings.PostProcessingSettings.TonemapMode);
		m_SceneCompositeMaterial->Set("u_Exposure", m_Settings.PostProcessingSettings.Exposure);
		m_SceneCompositeMaterial->Set("u_EnableBloom", (uint32)m_Settings.BloomSettings.Enable);

		m_RendererData.DebugShadowCascades = m_Settings.DebugView == DebugView::SHADOW_CASCADES ? 1 : 0;
		m_RendererData.DebugLightComplexity = m_Settings.DebugView == DebugView::LIGHT_COMPLEXITY ? 1 : 0;

		m_HBAOData.Intensity = m_Settings.AOSettings.Intensity;
		m_HBAOData.Bias = m_Settings.AOSettings.Bias;
		m_HBAOData.BlurSharpness = m_Settings.AOSettings.BlurSharpness;

		float projScale = float(m_ViewportSize.y) / (Math::Tan(cameraInfo.FOV * 0.5f) * 2.0f);
		float radius = m_Settings.AOSettings.Radius;
		m_HBAOData.NegInvR2 = -1.f / (radius * radius);
		m_HBAOData.RadiusToScreen = radius * 0.5f * projScale / 4.f;
		m_HBAOData.AOMultiplier = 1.0f / (1.0f - m_HBAOData.Bias);
		m_HBAOData.ProjInfo = m_CameraData.ProjInfo;

		m_SSRData.Intensity = m_Settings.SSRSettings.Intensity;
		m_SSRData.MaxRoughness = m_Settings.SSRSettings.MaxRoughness;
		m_SSRData.MaxSteps = m_Settings.SSRSettings.MaxSteps;
		m_SSRData.ScreenEdgesFade = m_Settings.SSRSettings.ScreenEdgesFade;
		m_SSRData.ConeTrace = m_Settings.SSRSettings.ConeTrace;
		m_SSRData.BackwardRays = m_Settings.SSRSettings.BackwardRays;
	}

	void SceneRenderer::EndScene()
	{
		ATN_PROFILE_FUNC();

		ResetStats();

		bool hasSelectedGeometry = m_SelectStaticGeometryList.Size() != 0 || m_SelectAnimGeometryList.Size() != 0;
		Antialising antialising = GetAntialising();

		m_Profiler->Reset();
		m_Profiler->BeginPipelineStatsQuery();

		{
			ATN_PROFILE_SCOPE("SceneRenderer::PreProcessMeshes");

			m_StaticGeometryList.Sort();
			m_AnimGeometryList.Sort();

			m_SelectStaticGeometryList.Sort();
			m_SelectAnimGeometryList.Sort();

			CalculateInstanceTransforms();
		}

		{
			ATN_PROFILE_SCOPE("SceneRenderer::UploadData");

			m_BonesSBO.Flush();
			m_TransformsStorage.Flush();
			Renderer::BindInstanceRateBuffer(m_RenderCommandBuffer, m_TransformsStorage.Get());

			m_CameraUBO->UploadData(&m_CameraData, sizeof(CameraData));
			m_RendererUBO->UploadData(&m_RendererData, sizeof(RendererData));
			m_LightSBO->UploadData(&m_LightData, sizeof(LightData));
			m_ShadowsUBO->UploadData(&m_ShadowsData, sizeof(ShadowsData));
			m_HBAO_UBO->UploadData(&m_HBAOData, sizeof(HBAOData));
			m_SSR_UBO->UploadData(&m_SSRData, sizeof(SSRData));
		}

		DirShadowMapPass();
		GBufferPass();
		HiZPass();
		LightCullingPass();
		HBAOPass();
		LightingPass();
		SkyboxPass();

		if (m_Settings.SSRSettings.Enable)
		{
			PreConvolutionPass();
			SSRPass();
		}

		if(m_Settings.BloomSettings.Enable)
			BloomPass();

		SceneCompositePass();

		if(hasSelectedGeometry)
			JumpFloodPass();

		Render2DPass();

		if (antialising == Antialising::FXAA)
			FXAAPass();
		else if (antialising == Antialising::SMAA)
			SMAAPass();

		m_Statistics.PipelineStats = m_Profiler->EndPipelineStatsQuery();
		m_Statistics.Meshes = m_StaticGeometryList.Size();
		m_Statistics.Instances = m_StaticGeometryList.GetInstancesCount();
		m_Statistics.AnimMeshes = m_AnimGeometryList.Size();

		m_StaticGeometryList.Clear();
		m_AnimGeometryList.Clear();
		m_SelectStaticGeometryList.Clear();
		m_SelectAnimGeometryList.Clear();
		m_BonesDataOffset = 0;
	}

	void SceneRenderer::DirShadowMapPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		m_DirShadowMapPass->Begin(commandBuffer);

		Renderer::BeginDebugRegion(commandBuffer, "StaticGeometry", { 0.8f, 0.4f, 0.2f, 1.f });
		{
			m_DirShadowMapStaticPipeline->Bind(commandBuffer);
			m_StaticGeometryList.FlushNoMaterials(commandBuffer, m_DirShadowMapStaticPipeline, true);
		}
		Renderer::EndDebugRegion(commandBuffer);

		Renderer::BeginDebugRegion(commandBuffer, "AnimatedGeometry", { 0.8f, 0.4f, 0.8f, 1.f });
		{
			m_DirShadowMapAnimPipeline->Bind(commandBuffer);
			m_AnimGeometryList.FlushNoMaterials(commandBuffer, m_DirShadowMapAnimPipeline, true);
		}
		Renderer::EndDebugRegion(commandBuffer);

		m_DirShadowMapPass->End(commandBuffer);
		m_Profiler->EndTimeQuery(&m_Statistics.DirShadowMapPass);
	}

	void SceneRenderer::GBufferPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		m_GBufferPass->Begin(commandBuffer);

		Renderer::BeginDebugRegion(commandBuffer, "StaticGeometry", { 0.8f, 0.4f, 0.2f, 1.f });
		{
			m_StaticGeometryPipeline->Bind(commandBuffer);
			m_StaticGeometryList.Flush(commandBuffer, m_StaticGeometryPipeline);
		}
		Renderer::EndDebugRegion(commandBuffer);

		Renderer::BeginDebugRegion(commandBuffer, "AnimatedGeometry", { 0.8f, 0.4f, 0.8f, 1.f });
		{
			m_AnimGeometryPipeline->Bind(commandBuffer);
			m_AnimGeometryList.Flush(commandBuffer, m_AnimGeometryPipeline);
		}
		Renderer::EndDebugRegion(commandBuffer);

		m_GBufferPass->End(commandBuffer);
		m_Profiler->EndTimeQuery(&m_Statistics.GBufferPass);
	}

	void SceneRenderer::HiZPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		m_HiZPass->Begin(commandBuffer);
		{
			m_HiZPipeline->Bind(commandBuffer);

			uint32 levelCount = Math::Min(m_HiZBuffer->GetMipLevelsCount(), (uint32)ShaderDef::HIZ_MIP_LEVEL_COUNT);

			for (uint32 i = 0; i < levelCount; ++i)
			{
				Ref<Material> material = m_HiZMaterials[i];
				Vector2 u_SourceSize = m_HiZBuffer->GetMipSize(i == 0 ? 0 : i - 1);
				Vector2 u_OutputSize = m_HiZBuffer->GetMipSize(i);

				material->Set("u_SourceSize", u_SourceSize);
				material->Set("u_OutputSize", u_OutputSize);
				material->Set("u_SourceTexelSize", Vector2(1.f) / u_SourceSize);
				material->Set("u_OutputTexelSize", Vector2(1.f) / u_OutputSize);

				material->Bind(commandBuffer);

				Vector2u mipSize = m_HiZBuffer->GetMipSize(i);
				Renderer::Dispatch(commandBuffer, m_HiZPipeline, { mipSize, 1 }, material);

				if (i != levelCount - 1)
					Renderer::InsertMemoryBarrier(commandBuffer);
			}
		}
		m_HiZPass->End(commandBuffer);
		m_Profiler->EndTimeQuery(&m_Statistics.HiZPass);
	}

	void SceneRenderer::LightCullingPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		m_LightCullingPass->Begin(commandBuffer);
		{
			m_LightCullingPipeline->Bind(commandBuffer);
			Renderer::Dispatch(commandBuffer, m_LightCullingPipeline, { m_ViewportSize, 1 });
		}
		m_LightCullingPass->End(commandBuffer);
		m_Profiler->EndTimeQuery(&m_Statistics.LightCullingPass);
	}

	void SceneRenderer::HBAOPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		if (!m_Settings.AOSettings.Enable)
		{
			// Clear
			m_HBAOBlurYPass->Begin(commandBuffer);
			m_HBAOBlurYPass->End(commandBuffer);
			return;
		}

		auto [quarterWidth, quarterHeight] = (m_ViewportSize + 3) / 4;

		m_Profiler->BeginTimeQuery();
		m_HBAODeinterleavePass->Begin(commandBuffer);
		{
			m_HBAODeinterleavePipeline->Bind(commandBuffer);
			Renderer::Dispatch(commandBuffer, m_HBAODeinterleavePipeline, { quarterWidth, quarterHeight, 1 });
		}
		m_HBAODeinterleavePass->End(commandBuffer);
		m_Profiler->EndTimeQuery(&m_Statistics.HBAODeinterleavePass);

		m_Profiler->BeginTimeQuery();
		m_HBAOComputePass->Begin(commandBuffer);
		{
			m_HBAOComputePipeline->Bind(commandBuffer);
			Renderer::Dispatch(commandBuffer, m_HBAOComputePipeline, { quarterWidth, quarterHeight, 16 });
		}
		m_HBAOComputePass->End(commandBuffer);
		m_Profiler->EndTimeQuery(&m_Statistics.HBAOComputePass);

		m_Profiler->BeginTimeQuery();
		Renderer::FullscreenPass(commandBuffer, m_HBAOBlurXPass, m_HBAOBlurXPipeline);
		Renderer::FullscreenPass(commandBuffer, m_HBAOBlurYPass, m_HBAOBlurYPipeline);
		m_Profiler->EndTimeQuery(&m_Statistics.HBAOBlurPass);
	}

	void SceneRenderer::LightingPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		Renderer::FullscreenPass(commandBuffer, m_DeferredLightingPass, m_DeferredLightingPipeline);
		m_Profiler->EndTimeQuery(&m_Statistics.DeferredLightingPass);
	}

	void SceneRenderer::SkyboxPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		Renderer::FullscreenPass(commandBuffer, m_SkyboxPass, m_SkyboxPipeline);
		m_Profiler->EndTimeQuery(&m_Statistics.SkyboxPass);
	}

	void SceneRenderer::PreConvolutionPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		m_PreConvolutionPass->Begin(commandBuffer);
		{
			m_PreConvolutionPipeline->Bind(commandBuffer);

			uint32 mipLevel = 0;
			for (uint32 i = 0; i < m_PreConvolutionMaterials.size(); ++i)
			{
				Ref<Material> material = m_PreConvolutionMaterials[i];
				material->Bind(commandBuffer);

				uint32 mipLevel = i == 0 ? 0 : (i + 2) / 3;
				Vector2u mipSize = m_HiColorBuffer->GetMipSize(mipLevel);
				Renderer::Dispatch(commandBuffer, m_PreConvolutionPipeline, { mipSize, 1 }, material);

				if(i != m_PreConvolutionMaterials.size() - 1)
					Renderer::InsertMemoryBarrier(commandBuffer);
			}
		}
		m_PreConvolutionPass->End(commandBuffer);
		m_Profiler->EndTimeQuery(&m_Statistics.PreConvolutionPass);
	}

	void SceneRenderer::SSRPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		Vector2i resolution = m_ViewportSize;

		if (m_Settings.SSRSettings.HalfRes)
			resolution = (resolution + 1) / 2;

		m_Profiler->BeginTimeQuery();
		m_SSRComputePass->Begin(commandBuffer);
		{
			m_SSRComputePipeline->Bind(commandBuffer);
			Renderer::Dispatch(commandBuffer, m_SSRComputePipeline, { resolution, 1 });
		}
		m_SSRComputePass->End(commandBuffer);
		m_Profiler->EndTimeQuery(&m_Statistics.SSRComputePass);


		m_Profiler->BeginTimeQuery();
		m_SSRCompositePass->Begin(commandBuffer);
		{
			m_SSRCompositePipeline->Bind(commandBuffer);
			Renderer::Dispatch(commandBuffer, m_SSRCompositePipeline, { m_ViewportSize, 1 });
		}
		m_SSRCompositePass->End(commandBuffer);
		m_Profiler->EndTimeQuery(&m_Statistics.SSRCompositePass);
	}

	void SceneRenderer::BloomPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		// Calculate number of downsample passes
		const uint32 downSampleResLimit = 8;

		uint32 width = m_HiColorBuffer->GetWidth();
		uint32 height = m_HiColorBuffer->GetHeight();
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
				material->Set("u_BloomTextureMip", m_HiColorBuffer->GetMipView(mip));
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

				Vector2u mipSize = m_HiColorBuffer->GetMipSize(mip);
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

				Vector2u mipSize = m_HiColorBuffer->GetMipSize(mip - 1);
				material->Set("u_TexelSize", Vector2(1.f, 1.f) / Vector2(mipSize));
				material->Set("u_ReadMipLevel", mip);
				material->Bind(commandBuffer);

				Renderer::Dispatch(commandBuffer, m_BloomUpsample, { mipSize.x, mipSize.y, 1 }, material);

				if (mip != 1)
					Renderer::InsertMemoryBarrier(commandBuffer);
			}
		}
		m_BloomPass->End(commandBuffer);
		m_Profiler->EndTimeQuery(&m_Statistics.BloomPass);
	}

	void SceneRenderer::SceneCompositePass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		Renderer::FullscreenPass(commandBuffer, m_SceneCompositePass, m_SceneCompositePipeline, m_SceneCompositeMaterial);
		m_Profiler->EndTimeQuery(&m_Statistics.SceneCompositePass);
	}

	void SceneRenderer::JumpFloodPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		Renderer::BeginDebugRegion(commandBuffer, "JumpFlood", { 0.9f, 0.5f, 0.3f, 1.f });

		m_JumpFloodSilhouettePass->Begin(commandBuffer);
		{
			m_JFSilhouetteStaticPipeline->Bind(commandBuffer);
			m_SelectStaticGeometryList.FlushNoMaterials(commandBuffer, m_JFSilhouetteStaticPipeline);

			m_JFSilhouetteAnimPipeline->Bind(commandBuffer);
			m_SelectAnimGeometryList.FlushNoMaterials(commandBuffer, m_JFSilhouetteAnimPipeline);
		}
		m_JumpFloodSilhouettePass->End(commandBuffer);

		Renderer::FullscreenPass(commandBuffer, m_JumpFloodInitPass, m_JumpFloodInitPipeline);

		// calculate the number of jump flood passes needed for the current outline width
		// + 1.0f to handle half pixel inset of the init pass and antialiasing
		uint32 jumps = Math::Ceil(Math::Log2(m_OutlineWidth + 1.f)) - 1;
		uint32 index = 1; // Init pass initializes 0 index texture

		for(int32 i = jumps; i >= 0; --i)
		{
			int stepWidth = Math::Pow(2, i) + 0.5f;

			m_JumpFloodMaterial->Set("u_StepWidth", stepWidth);
			Renderer::FullscreenPass(commandBuffer, m_JumpFloodPasses[index], m_JumpFloodPipelines[index], m_JumpFloodMaterial);

			index = (index + 1) % 2;
		}

		Renderer::FullscreenPass(commandBuffer, m_JumpFloodCompositePass, m_JumpFloodCompositePipeline, m_JumpFloodCompositeMaterial);

		Renderer::EndDebugRegion(commandBuffer);
		m_Profiler->EndTimeQuery(&m_Statistics.JumpFloodPass);
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
		m_Profiler->EndTimeQuery(&m_Statistics.Render2DPass);
	}

	void SceneRenderer::FXAAPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();
		m_FXAAPass->Begin(commandBuffer);
		{
			m_FXAAPipeline->Bind(commandBuffer);
			Renderer::Dispatch(commandBuffer, m_FXAAPipeline, { m_ViewportSize.x, m_ViewportSize.y, 1 });
		}
		m_FXAAPass->End(commandBuffer);
		m_Profiler->EndTimeQuery(&m_Statistics.AAPass);
	}

	void SceneRenderer::SMAAPass()
	{
		auto commandBuffer = m_RenderCommandBuffer;

		m_Profiler->BeginTimeQuery();

		Renderer::FullscreenPass(commandBuffer, m_SMAAEdgesPass, m_SMAAEdgesPipeline);
		Renderer::FullscreenPass(commandBuffer, m_SMAAWeightsPass, m_SMAAWeightsPipeline);
		Renderer::FullscreenPass(commandBuffer, m_SMAABlendingPass, m_SMAABlendingPipeline);

		m_Profiler->EndTimeQuery(&m_Statistics.AAPass);
	}

	void SceneRenderer::CalculateCascadeLightSpaces(DirectionalLight& light)
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
		float frustumSize = 0.f;

		// Build light space matrices
		for (uint32 layer = 0; layer < ShaderDef::SHADOW_CASCADES_COUNT; ++layer)
		{
			float cascadeFar = m_ShadowsData.CascadePlanes[layer].y;
			// convert to range (0, 1]
			float split = (cascadeFar - cameraNear) / (cameraFar - cameraNear);

			std::array<Vector3, 8> frustumCorners = {
				Vector3{  1.0f, -1.0f, 1.f },
				Vector3{ -1.0f, -1.0f, 1.f },
				Vector3{  1.0f,  1.0f, 1.f },
				Vector3{ -1.0f,  1.0f, 1.f },

				Vector3{  1.0f, -1.0f, 0.0f },
				Vector3{ -1.0f, -1.0f, 0.0f },
				Vector3{  1.0f,  1.0f, 0.0f },
				Vector3{ -1.0f,  1.0f, 0.0f },
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
			Matrix4 lightProjection = Math::Ortho(minExtents.x, maxExtents.x, maxExtents.y, minExtents.y, 0.f, minExtents.z - maxExtents.z);

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

			frustumSize = Math::Max(frustumSize, maxExtents.z - minExtents.z);

			lastSplit = split;
		}

		// ehhhh
		light.LightSize /= 8.f;
	}

	void SceneRenderer::CalculateInstanceTransforms()
	{
		std::vector<InstanceTransformData> transformData;

		m_StaticGeometryList.SetInstanceOffset(0);
		m_StaticGeometryList.EmplaceInstanceTransforms(transformData);

		m_AnimGeometryList.SetInstanceOffset(transformData.size());
		m_AnimGeometryList.EmplaceInstanceTransforms(transformData);

		m_SelectStaticGeometryList.SetInstanceOffset(transformData.size());
		m_SelectStaticGeometryList.EmplaceInstanceTransforms(transformData);

		m_SelectAnimGeometryList.SetInstanceOffset(transformData.size());
		m_SelectAnimGeometryList.EmplaceInstanceTransforms(transformData);

		m_TransformsStorage.Push(transformData.data(), transformData.size() * sizeof(InstanceTransformData));
	}

	void SceneRenderer::ResetStats()
	{
		Time gpuTime = m_Statistics.DirShadowMapPass + 
			m_Statistics.GBufferPass + 
			m_Statistics.HiZPass +
			m_Statistics.LightCullingPass + 
			m_Statistics.HBAODeinterleavePass +
			m_Statistics.HBAOComputePass +
			m_Statistics.HBAOBlurPass +
			m_Statistics.DeferredLightingPass +
			m_Statistics.SkyboxPass + 
			m_Statistics.PreConvolutionPass +
			m_Statistics.SSRComputePass +
			m_Statistics.SSRCompositePass +
			m_Statistics.BloomPass + 
			m_Statistics.SceneCompositePass +
			m_Statistics.JumpFloodPass +
			m_Statistics.Render2DPass + 
			m_Statistics.AAPass;

		memset(&m_Statistics, 0, sizeof(m_Statistics));
		m_Statistics.GPUTime = gpuTime;
	}

	void SceneRenderer::ApplySettings()
	{
		OnViewportResize(m_OriginalViewportSize.x, m_OriginalViewportSize.y);
	}
}
