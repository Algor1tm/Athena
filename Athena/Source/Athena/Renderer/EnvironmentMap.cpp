#include "EnvironmentMap.h"
#include "Athena/Renderer/ComputePipeline.h"
#include "Athena/Renderer/ComputePass.h"
#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	Ref<EnvironmentMap> EnvironmentMap::Create(const FilePath& hdrMap, uint32 resolution)
	{
		Ref<EnvironmentMap> result = Ref<EnvironmentMap>::Create();
		result->m_FilePath = hdrMap;

		result->SetResolution(resolution);

		return result;
	}

	void EnvironmentMap::SetResolution(uint32 resolution)
	{
		m_Resolution = resolution;
		Load();
	}

	void EnvironmentMap::Load()
	{
		Ref<Texture2D> panorama = Texture2D::Create(m_FilePath);
		
		TextureCubeCreateInfo cubemapInfo;
		cubemapInfo.Name = std::format("{}_Prefiltered", m_FilePath.stem().string());
		cubemapInfo.Format = ImageFormat::R11G11B10F;
		cubemapInfo.Usage = ImageUsage(ImageUsage::STORAGE | ImageUsage::SAMPLED);
		cubemapInfo.InitialData = nullptr;
		cubemapInfo.Width = m_Resolution;
		cubemapInfo.Height = m_Resolution;
		cubemapInfo.MipLevels = ShaderDef::MAX_SKYBOX_MAP_LOD;
		cubemapInfo.SamplerInfo.MinFilter = TextureFilter::LINEAR;
		cubemapInfo.SamplerInfo.MagFilter = TextureFilter::LINEAR;
		cubemapInfo.SamplerInfo.MipMapFilter = TextureFilter::LINEAR;
		cubemapInfo.SamplerInfo.Wrap = TextureWrap::CLAMP_TO_EDGE;

		m_PrefilteredMap = TextureCube::Create(cubemapInfo);

		cubemapInfo.Name = std::format("{}_Irradiance", m_FilePath.stem().string());
		cubemapInfo.Width = m_IrradianceMapResolution;
		cubemapInfo.Height = m_IrradianceMapResolution;
		cubemapInfo.MipLevels = 1;

		m_IrradianceMap = TextureCube::Create(cubemapInfo);

		ComputePassCreateInfo passInfo;
		passInfo.Name = "EnvMapGenerationPass";
		passInfo.Outputs.push_back(m_PrefilteredMap->GetImage());
		passInfo.Outputs.push_back(m_IrradianceMap->GetImage());

		Ref<ComputePass> pass = ComputePass::Create(passInfo);

		// Panorama To Cubemap Pipeline
		ComputePipelineCreateInfo pipelineInfo;
		pipelineInfo.Name = "PanoramaToCubemapPipeline";
		pipelineInfo.Shader = Renderer::GetShaderPack()->Get("PanoramaToCubemap");
		pipelineInfo.WorkGroupSize = { 8, 4, 1 };

		Ref<ComputePipeline> panoramaToCubePipeline = ComputePipeline::Create(pipelineInfo);
		panoramaToCubePipeline->SetInput("u_PanoramaTex", panorama);
		panoramaToCubePipeline->SetInput("u_Cubemap", m_PrefilteredMap);
		panoramaToCubePipeline->Bake();

		// Irradiance Pipeline
		pipelineInfo.Name = "IrradianceMapPipeline";
		pipelineInfo.Shader = Renderer::GetShaderPack()->Get("IrradianceMapConvolution");
		pipelineInfo.WorkGroupSize = { 8, 4, 1 };
		
		Ref<ComputePipeline> irradiancePipeline = ComputePipeline::Create(pipelineInfo);
		irradiancePipeline->SetInput("u_Cubemap", m_PrefilteredMap);
		irradiancePipeline->SetInput("u_IrradianceMap", m_IrradianceMap);
		irradiancePipeline->Bake();

		RenderCommandBufferCreateInfo cmdBufferInfo;
		cmdBufferInfo.Name = "EnvMapGeneration";
		cmdBufferInfo.Usage = RenderCommandBufferUsage::IMMEDIATE;

		Ref<RenderCommandBuffer> commandBuffer = RenderCommandBuffer::Create(cmdBufferInfo);

		commandBuffer->Begin();
		{
			pass->Begin(commandBuffer);

			panoramaToCubePipeline->Bind(commandBuffer);
			Renderer::Dispatch(commandBuffer, panoramaToCubePipeline, { m_Resolution, m_Resolution, 6 });

			irradiancePipeline->Bind(commandBuffer);
			Renderer::Dispatch(commandBuffer, irradiancePipeline, { m_IrradianceMapResolution, m_IrradianceMapResolution, 6 });

			pass->End(commandBuffer);
		}
		commandBuffer->End();
		commandBuffer->Submit();

		m_PrefilteredMap->GenerateMipMap(ShaderDef::MAX_SKYBOX_MAP_LOD);
	}
}
