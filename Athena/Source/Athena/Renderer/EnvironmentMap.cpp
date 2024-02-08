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


		ComputePassCreateInfo passInfo;
		passInfo.Name = "PanoramaToCubemapPass";
		passInfo.Outputs.push_back(m_PrefilteredMap->GetImage());

		Ref<ComputePass> pass = ComputePass::Create(passInfo);

		ComputePipelineCreateInfo pipelineInfo;
		pipelineInfo.Name = "PanoramaToCubemapPipeline";
		pipelineInfo.Shader = Renderer::GetShaderPack()->Get("PanoramaToCubemap");
		pipelineInfo.WorkGroupSize = { 8, 4, 1 };

		Ref<ComputePipeline> pipeline = ComputePipeline::Create(pipelineInfo);
		pipeline->SetInput("u_PanoramaTex", panorama);
		pipeline->SetInput("u_Cubemap", m_PrefilteredMap);
		pipeline->Bake();

		RenderCommandBufferCreateInfo cmdBufferInfo;
		cmdBufferInfo.Name = "EnvironmentMapGeneration";
		cmdBufferInfo.Usage = RenderCommandBufferUsage::IMMEDIATE;

		Ref<RenderCommandBuffer> commandBuffer = RenderCommandBuffer::Create(cmdBufferInfo);

		commandBuffer->Begin();
		{
			pass->Begin(commandBuffer);

			pipeline->Bind(commandBuffer);
			Renderer::Dispatch(commandBuffer, pipeline, { m_Resolution, m_Resolution, 6});

			pass->End(commandBuffer);
		}
		commandBuffer->End();
		commandBuffer->Submit();

		m_PrefilteredMap->GenerateMipMap(ShaderDef::MAX_SKYBOX_MAP_LOD);
	}
}
