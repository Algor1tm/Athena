#include "EnvironmentMap.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Application.h"
#include "Athena/Asset/Editor/TextureImporter.h"
#include "Athena/Renderer/ComputePass.h"
#include "Athena/Renderer/ComputePipeline.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/EngineTextures.h"


namespace Athena
{
	Ref<TextureCube> EnvironmentMap::s_CachedEnvMap;
	Ref<TextureCube> EnvironmentMap::s_CachedIrradiance;
	PreethamParams EnvironmentMap::s_CachedPreethamParams;

	EnvironmentMap::EnvironmentMap()
	{

	}

	Ref<TextureCube> EnvironmentMap::GetEnvironmentTexture()
	{
		return m_EnvironmentTexture ? m_EnvironmentTexture : EngineTextures::GetBlackTextureCube();
	}

	Ref<TextureCube> EnvironmentMap::GetIrradianceTexture()
	{
		return m_IrradianceTexture ? m_IrradianceTexture : EngineTextures::GetBlackTextureCube();
	}

	bool EnvironmentMap::Serialize(const FilePath& absolutePath) const
	{
		return true;
	}

	bool EnvironmentMap::Deserialize(const FilePath& absolutePath, Ref<AssetImportSettings> importSettings)
	{
		// Cannnot execute rendering pipeline in multiple threads(main thread and asset watcher thread)
		Application::Get().SubmitToMainThread([this, absolutePath, importSettings]() 
		{
			// Load hdr texture
			Ref<TextureImportSettings> settings = Ref<TextureImportSettings>::Create();
			settings->sRGB = false;
			settings->GenerateMipMaps = false;
			settings->FilterMode = TextureFilter::LINEAR;

			TextureImporter importer(settings);
			Ref<Texture2D> panorama = importer.Import(absolutePath);

			Ref<EnvironmentMapImportSettings> envImportSettings = importSettings.As<EnvironmentMapImportSettings>();
			CreateTextures(envImportSettings->Resolution, envImportSettings->FloatFormat, m_EnvironmentTexture, m_IrradianceTexture);

			// Create cube map from this texture
			ComputePassCreateInfo passInfo;
			passInfo.Name = "PanoramaToCubePass";
			passInfo.DebugColor = { 0.2f, 0.4f, 0.6f };

			Ref<ComputePass> panoramaToCubePass = ComputePass::Create(passInfo);
			panoramaToCubePass->SetOutput(m_EnvironmentTexture);
			panoramaToCubePass->Bake();

			Ref<ComputePipeline> panoramaToCubePipeline = ComputePipeline::Create(Renderer::GetShaderPack()->Get("PanoramaToCubemap"));
			panoramaToCubePipeline->SetInput("u_PanoramaTex", EngineTextures::GetWhiteTexture());
			panoramaToCubePipeline->SetInput("u_Cubemap", m_EnvironmentTexture);
			panoramaToCubePipeline->Bake();

			panoramaToCubePipeline->SetInput("u_PanoramaTex", panorama);


			RenderCommandBufferCreateInfo info;
			info.Name = "EnvironmentMap";
			info.Usage = RenderCommandBufferUsage::IMMEDIATE;
			Ref<RenderCommandBuffer> commandBuffer = RenderCommandBuffer::Create(info);
			commandBuffer->Begin();

			panoramaToCubePass->Begin(commandBuffer);
			{
				panoramaToCubePipeline->Bind(commandBuffer);
				Renderer::Dispatch(commandBuffer, panoramaToCubePipeline, { m_Resolution, m_Resolution, 6 });
			}
			panoramaToCubePass->End(commandBuffer);

			// Filter
			FilterEnvironmentMap(commandBuffer, m_EnvironmentTexture, m_IrradianceTexture);

			commandBuffer->End();
			commandBuffer->Submit(false);
		});

		return true;
	}

	void EnvironmentMap::CreatePreethamMap(const PreethamParams& params, Ref<TextureCube>& outEnvTex, Ref<TextureCube>& outIrradianceTex)
	{
		if (s_CachedPreethamParams == params && s_CachedEnvMap && s_CachedIrradiance)
		{
			outEnvTex = s_CachedEnvMap;
			outIrradianceTex = s_CachedIrradiance;
			return;
		}

		CreateTextures(params.Resolution, Format::R11G11B10F, outEnvTex, outIrradianceTex);

		// Generate cube map texture
		ComputePassCreateInfo passInfo;
		passInfo.Name = "PreethamPass";
		passInfo.DebugColor = { 0.6f, 0.4f, 0.2f, 1.f };

		Ref<ComputePass> preethamPass = ComputePass::Create(passInfo);
		preethamPass->SetOutput(outEnvTex);
		preethamPass->Bake();

		Ref<ComputePipeline> preethamPipeline = ComputePipeline::Create(Renderer::GetShaderPack()->Get("PreethamSky"));
		preethamPipeline->SetInput("u_EnvironmentMap", outEnvTex);
		preethamPipeline->Bake();

		Ref<Material> preethamMaterial = Material::Create(preethamPipeline->GetShader(), preethamPipeline->GetName());

		RenderCommandBufferCreateInfo info;
		info.Name = "EnvironmentMap";
		info.Usage = RenderCommandBufferUsage::IMMEDIATE;
		Ref<RenderCommandBuffer> commandBuffer = RenderCommandBuffer::Create(info);
		commandBuffer->Begin();

		preethamPass->Begin(commandBuffer);
		{
			preethamPipeline->Bind(commandBuffer);

			preethamMaterial->Set("u_Turbidity", params.Turbidity);
			preethamMaterial->Set("u_Azimuth", params.Azimuth);
			preethamMaterial->Set("u_Inclination", params.Inclination);
			preethamMaterial->Bind(commandBuffer);

			Renderer::Dispatch(commandBuffer, preethamPipeline, { params.Resolution, params.Resolution, 6 }, preethamMaterial);
		}
		preethamPass->End(commandBuffer);

		// Filter
		FilterEnvironmentMap(commandBuffer, outEnvTex, outIrradianceTex);

		commandBuffer->End();
		commandBuffer->Submit(false);

		s_CachedPreethamParams = params;
		s_CachedEnvMap = outEnvTex;
		s_CachedIrradiance = outIrradianceTex;
	}

	void EnvironmentMap::CreateTextures(float resolution, Format floatFormat, Ref<TextureCube>& outEnvTex, Ref<TextureCube>& outIrradianceTex)
	{
		ATN_CORE_ASSERT(FormatUtils::IsHDRFormat(floatFormat) && (FormatUtils::BytesPerPixel(floatFormat) % 3 != 0), "Invalid environment map format!");

		TextureCreateInfo cubemapInfo;
		cubemapInfo.Name = "EnvironmentMap";
		cubemapInfo.TextureFormat = floatFormat;
		cubemapInfo.Usage = TextureUsage(TextureUsage::STORAGE | TextureUsage::SAMPLED);
		cubemapInfo.Width = resolution;
		cubemapInfo.Height = resolution;
		cubemapInfo.GenerateMipMap = true;
		cubemapInfo.Sampler.Filter = TextureFilter::TRILINEAR;
		cubemapInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

		outEnvTex = TextureCube::Create(cubemapInfo);

		cubemapInfo.Name = "EnvIrradianceMap";
		cubemapInfo.TextureFormat = Format::R11G11B10F;
		cubemapInfo.Width = s_IrradianceMapResolution;
		cubemapInfo.Height = s_IrradianceMapResolution;
		cubemapInfo.GenerateMipMap = false;

		outIrradianceTex = TextureCube::Create(cubemapInfo);
	}

	void EnvironmentMap::FilterEnvironmentMap(const Ref<RenderCommandBuffer>& commandBuffer, Ref<TextureCube> envTex, Ref<TextureCube> irradianceTex)
	{
		// Linear mip maps
		Renderer::BeginDebugRegion(commandBuffer, "EnvironmentBlitMipMap", { 0.6f, 0.4f, 0.2f, 1.f });
		{
			Renderer::BlitMipMap(commandBuffer, envTex);
		}
		Renderer::EndDebugRegion(commandBuffer);

		// Irradiance Pipeline
		{
			ComputePassCreateInfo passInfo;
			passInfo.Name = "IrradiancePass";
			passInfo.DebugColor = { 0.6f, 0.4f, 0.2f, 1.f };

			Ref<ComputePass> irradiancePass = ComputePass::Create(passInfo);
			irradiancePass->SetOutput(irradianceTex);
			irradiancePass->Bake();

			Ref<ComputePipeline> irradiancePipeline = ComputePipeline::Create(Renderer::GetShaderPack()->Get("IrradianceMapConvolution"));
			irradiancePipeline->SetInput("u_Cubemap", envTex);
			irradiancePipeline->SetInput("u_IrradianceMap", irradianceTex);
			irradiancePipeline->Bake();

			irradiancePass->Begin(commandBuffer);
			{
				irradiancePipeline->Bind(commandBuffer);
				Renderer::Dispatch(commandBuffer, irradiancePipeline, { s_IrradianceMapResolution, s_IrradianceMapResolution, 6 });
			}
			irradiancePass->End(commandBuffer);
		}

		// Mip Filter Pipeline
		{
			ComputePassCreateInfo passInfo;
			passInfo.Name = "MipFilterPass";
			passInfo.DebugColor = { 0.6f, 0.4f, 0.2f, 1.f };

			Ref<ComputePass> mipFilterPass = ComputePass::Create(passInfo);
			mipFilterPass->SetOutput(envTex);
			mipFilterPass->Bake();

			Ref<Shader> shader = Renderer::GetShaderPack()->Get("EnvironmentMipFilter");

			Ref<ComputePipeline> pipFilterPipeline = ComputePipeline::Create(shader);
			pipFilterPipeline->SetInput("u_EnvironmentMap", envTex);
			pipFilterPipeline->Bake();

			std::array<Ref<Material>, ShaderDef::MAX_SKYBOX_MAP_LOD> mipFilterMaterials;
			for (uint32 mip = 1; mip < ShaderDef::MAX_SKYBOX_MAP_LOD; ++mip)
			{
				Ref<Material> mipMaterial = Material::Create(shader, std::format("{}_{}", shader->GetName(), mip - 1));
				mipMaterial->Set("u_EnvironmentMipImage", envTex->GetMipView(mip));
				mipMaterial->Set("u_MipLevel", mip);

				mipFilterMaterials[mip] = mipMaterial;
			}

			mipFilterPass->Begin(commandBuffer);
			pipFilterPipeline->Bind(commandBuffer);
			for (uint32 mip = 1; mip < ShaderDef::MAX_SKYBOX_MAP_LOD; ++mip)
			{
				uint32 mipResolution = envTex->GetInfo().Width * Math::Pow(0.5f, (float)mip);

				mipFilterMaterials[mip]->Bind(commandBuffer);
				Renderer::Dispatch(commandBuffer, pipFilterPipeline, { mipResolution, mipResolution, 6 }, mipFilterMaterials[mip]);

				if (mip != ShaderDef::MAX_SKYBOX_MAP_LOD - 1)
					Renderer::InsertMemoryBarrier(commandBuffer);
			}
			mipFilterPass->End(commandBuffer);
		}
	}

	void EnvironmentMap::ClearCache()
	{
		s_CachedEnvMap.Release();
		s_CachedIrradiance.Release();
	}
}
