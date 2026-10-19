#include "EngineRenderAssets.h"

#include "Athena/Core/Application.h"
#include "Athena/Asset/Editor/TextureImporter.h"
#include "Athena/Asset/Editor/FontImporter.h"
#include "Athena/Renderer/RenderThread.h"
#include "Athena/Math/Random.h"


namespace Athena
{
	void EngineRenderAssets::Init()
	{
		Buffer texData = Buffer(sizeof(uint32));

		uint32 whiteTextureData = 0xffffffff;
		texData.Write(&whiteTextureData, sizeof(uint32));

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

		WhiteTexture = Texture::Create(texInfo, &texData);

		uint32 blackTextureData = 0xff000000;
		texData.Write(&blackTextureData, sizeof(uint32));

		texInfo.Name = "Renderer_BlackTexture";

		BlackTexture = Texture::Create(texInfo, &texData);

		TextureCreateInfo texCubeInfo;
		texCubeInfo.Name = "Renderer_BlackTextureCube";
		texCubeInfo.Type = TextureType::TEXTURE_CUBE;
		texCubeInfo.TextureFormat = Format::RGBA8;
		texCubeInfo.Usage = TextureUsage(TextureUsage::SAMPLED | TextureUsage::STORAGE);
		texCubeInfo.Width = 1;
		texCubeInfo.Height = 1;
		texCubeInfo.Layers = 6;
		texCubeInfo.GenerateMipMap = false;
		texCubeInfo.Sampler.Filter = TextureFilter::NEAREST;
		texCubeInfo.Sampler.Wrap = TextureWrap::REPEAT;

		BlackTextureCube = TextureCube::Create(texCubeInfo, &texData);

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

			BlueNoise = importer.Import(path);
		}

		// SMAA Area texture
		{
			FilePath path = resourcesPath / "Textures/SMAA-AreaTex.png";

			importSettings->Name = "Renderer_SMAA-AreaTex";
			importSettings->ExtractChannelsNum = 4;

			SMAA_AreaLUT = importer.Import(path);
		}

		// SMAA Search texture
		{
			FilePath path = resourcesPath / "Textures/SMAA-SearchTex.png";

			importSettings->Name = "Renderer_SMAA-SearchTex";
			importSettings->ExtractChannelsNum = 4;

			SMAA_SearchLUT = importer.Import(path);
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

			BRDF_LUT = Texture2D::Create(texInfo);

			//ComputePassCreateInfo passInfo;
			//passInfo.Name = "BRDF_LUT_Pass";

			//Ref<ComputePass> pass = ComputePass::Create(passInfo);
			//pass->SetOutput(s_Data.BRDF_LUT);
			//pass->Bake();

			//Ref<ComputePipeline> pipeline = ComputePipeline::Create(Renderer::GetShaderPack()->Get("BRDF_LUT"));
			//pipeline->SetInput("u_BRDF_LUT", s_Data.BRDF_LUT);
			//pipeline->Bake();

			//RenderCommandBufferCreateInfo cmdBufferInfo;
			//cmdBufferInfo.Name = "BRDF_LUT_Generation";
			//cmdBufferInfo.Usage = RenderCommandBufferUsage::IMMEDIATE;

			//Ref<RenderCommandBuffer> commandBuffer = RenderCommandBuffer::Create(cmdBufferInfo);

			//commandBuffer->Begin();
			//{
			//	pass->Begin(commandBuffer);
			//	pipeline->Bind(commandBuffer);
			//	Renderer::Dispatch(commandBuffer, pipeline, { texInfo.Width, texInfo.Height, 1 });
			//	pass->End(commandBuffer);
			//}
			//commandBuffer->End();
			//commandBuffer->Submit(false);
		}

		DefaultTextureAsset = Ref<TextureAsset>::Create(WhiteTexture);
		DefaultMaterialAsset = Ref<MaterialAsset>::Create();
		
		FontImporter fontImporter;
		DefaultFontAsset = Ref<Font>::Create();
		fontImporter.Import(DefaultFontAsset, resourcesPath / "Fonts/Open_Sans/OpenSans-Medium.ttf");
	}

	void EngineRenderAssets::Destroy()
	{
		ENQUEUE_RENDER_COMMAND([this]()
			{
				WhiteTexture.Release();
				BlackTexture.Release();
				BlueNoise.Release();
				SMAA_AreaLUT.Release();
				SMAA_SearchLUT.Release();
				BRDF_LUT.Release();
				BlackTextureCube.Release();
			});

		DefaultTextureAsset.Release();
		DefaultMaterialAsset.Release();
		DefaultFontAsset.Release();
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
