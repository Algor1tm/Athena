#include "EngineTextures.h"

#include "Athena/Core/Application.h"
#include "Athena/Asset/Editor/TextureImporter.h"
#include "Athena/Math/Random.h"
#include "Athena/Renderer/ComputePass.h"
#include "Athena/Renderer/ComputePipeline.h"
#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	struct TextureGeneratorData
	{
		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BlackTexture;
		Ref<TextureCube> BlackTextureCube;

		Ref<Texture2D> BlueNoise;
		Ref<Texture2D> SMAA_AreaLUT;
		Ref<Texture2D> SMAA_SearchLUT;
		Ref<Texture2D> BRDF_LUT;
	};

	static TextureGeneratorData s_Data;


	void EngineTextures::Init()
	{
		uint32 whiteTextureData = 0xffffffff;
		Buffer texData = Buffer::Move(&whiteTextureData, sizeof(uint32));

		TextureCreateInfo texInfo;
		texInfo.Name = "Renderer_WhiteTexture";
		texInfo.TextureFormat = Format::RGBA8;
		texInfo.Usage = TextureUsage(TextureUsage::SAMPLED | TextureUsage::STORAGE);
		texInfo.Width = 1;
		texInfo.Height = 1;
		texInfo.Layers = 1;
		texInfo.GenerateMipMap = false;
		texInfo.Sampler.Filter = TextureFilter::NEAREST;
		texInfo.Sampler.Wrap = TextureWrap::REPEAT;

		s_Data.WhiteTexture = Texture2D::Create(texInfo, texData);

		uint32 blackTextureData = 0xff000000;
		texData.Write(&blackTextureData, sizeof(uint32));

		texInfo.Name = "Renderer_BlackTexture";

		s_Data.BlackTexture = Texture2D::Create(texInfo, texData);

		TextureCreateInfo texCubeInfo;
		texCubeInfo.Name = "Renderer_BlackTextureCube";
		texCubeInfo.TextureFormat = Format::RGBA8;
		texCubeInfo.Usage = TextureUsage(TextureUsage::SAMPLED | TextureUsage::STORAGE);
		texCubeInfo.Width = 1;
		texCubeInfo.Height = 1;
		texCubeInfo.GenerateMipMap = false;
		texCubeInfo.Sampler.Filter = TextureFilter::NEAREST;
		texCubeInfo.Sampler.Wrap = TextureWrap::REPEAT;

		s_Data.BlackTextureCube = TextureCube::Create(texCubeInfo, texData);

		const FilePath& resourcesPath = Application::Get().GetConfig().EngineResourcesPath;

		Ref<TextureImportSettings> importSettings = Ref<TextureImportSettings>::Create();
		importSettings->GenerateMipMaps = false;
		importSettings->sRGB = false;
		importSettings->FilterMode = TextureFilter::LINEAR;
		importSettings->WrapMode = TextureWrap::REPEAT;
		importSettings->AnisotropyLevel = 0.f;

		TextureImporter importer(importSettings);

		// BLUE NOISE
		{
			FilePath path = resourcesPath / "Textures/BlueNoise16x16.png";

			importSettings->Name = "Renderer_BlueNoise";
			importSettings->ExtractChannelsNum = 1;

			s_Data.BlueNoise = importer.Import(path);
		}

		// SMAA Area texture
		{
			FilePath path = resourcesPath / "Textures/SMAA-AreaTex.png";

			importSettings->Name = "Renderer_SMAA-AreaTex";
			importSettings->ExtractChannelsNum = 4;

			s_Data.SMAA_AreaLUT = importer.Import(path);
		}

		// SMAA Search texture
		{
			FilePath path = resourcesPath / "Textures/SMAA-SearchTex.png";

			importSettings->Name = "Renderer_SMAA-SearchTex";
			importSettings->ExtractChannelsNum = 4;

			s_Data.SMAA_SearchLUT = importer.Import(path);
		}

		// BRDF_LUT
		{
			texInfo.Name = "Renderer_BRDF_LUT";
			texInfo.TextureFormat = Format::RG16F;
			texInfo.Usage = TextureUsage(TextureUsage::STORAGE | TextureUsage::SAMPLED);
			texInfo.Width = 512;
			texInfo.Height = 512;
			texInfo.Layers = 1;
			texInfo.GenerateMipMap = false;
			texInfo.Sampler.Filter = TextureFilter::LINEAR;
			texInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

			s_Data.BRDF_LUT = Texture2D::Create(texInfo);

			ComputePassCreateInfo passInfo;
			passInfo.Name = "BRDF_LUT_Pass";

			Ref<ComputePass> pass = ComputePass::Create(passInfo);
			pass->SetOutput(s_Data.BRDF_LUT);
			pass->Bake();

			Ref<ComputePipeline> pipeline = ComputePipeline::Create(Renderer::GetShaderPack()->Get("BRDF_LUT"));
			pipeline->SetInput("u_BRDF_LUT", s_Data.BRDF_LUT);
			pipeline->Bake();

			RenderCommandBufferCreateInfo cmdBufferInfo;
			cmdBufferInfo.Name = "BRDF_LUT_Generation";
			cmdBufferInfo.Usage = RenderCommandBufferUsage::IMMEDIATE;

			Ref<RenderCommandBuffer> commandBuffer = RenderCommandBuffer::Create(cmdBufferInfo);

			commandBuffer->Begin();
			{
				pass->Begin(commandBuffer);
				pipeline->Bind(commandBuffer);
				Renderer::Dispatch(commandBuffer, pipeline, { texInfo.Width, texInfo.Height, 1 });
				pass->End(commandBuffer);
			}
			commandBuffer->End();
			commandBuffer->Submit(false);
		}
	}

	void EngineTextures::Shutdown()
	{
		s_Data.WhiteTexture.Release();
		s_Data.BlackTexture.Release();
		s_Data.BlueNoise.Release();
		s_Data.SMAA_AreaLUT.Release();
		s_Data.SMAA_SearchLUT.Release();
		s_Data.BRDF_LUT.Release();
		s_Data.BlackTextureCube.Release();
	}

	Ref<Texture2D> EngineTextures::GetBRDF_LUT()
	{
		return s_Data.BRDF_LUT;
	}

	Ref<Texture2D> EngineTextures::GetSMAA_AreaLUT()
	{
		return s_Data.SMAA_AreaLUT;
	}

	Ref<Texture2D> EngineTextures::GetSMAA_SearchLUT()
	{
		return s_Data.SMAA_SearchLUT;
	}

	Ref<Texture2D> EngineTextures::GetWhiteTexture()
	{
		return s_Data.WhiteTexture;
	}

	Ref<Texture2D> EngineTextures::GetBlackTexture()
	{
		return s_Data.BlackTexture;
	}

	Ref<TextureCube> EngineTextures::GetBlackTextureCube()
	{
		return s_Data.BlackTextureCube;
	}

	Ref<Texture2D> EngineTextures::GetBlueNoise()
	{
		return s_Data.BlueNoise;
	}

	std::vector<Vector4> Noise::GetHBAOJitters(uint32 numSamples)
	{
		std::vector<Vector4> result(numSamples);

		const uint32 numDir = 8;
		for (int i = 0; i < numSamples; i++)
		{
			// Use random rotation angles in [0,2PI/NUM_DIRECTIONS)
			float rand1 = Math::Random::Float(0, 1);
			float rand2 = Math::Random::Float(0, 1);

			float angle = 2 * Math::PI<float>() * rand1 / numDir;

			result[i].x = Math::Cos(angle);
			result[i].y = Math::Sin(angle);
			result[i].z = rand2;
			result[i].w = 0;
		}

		return result;
	}
}
