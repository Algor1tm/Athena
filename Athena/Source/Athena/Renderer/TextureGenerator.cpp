#include "TextureGenerator.h"

#include "Athena/Core/Application.h"
#include "Athena/Asset/TextureImporter.h"
#include "Athena/Renderer/ComputePass.h"
#include "Athena/Renderer/ComputePipeline.h"
#include "Athena/Renderer/Renderer.h"

#include <stb_image/stb_image.h>

namespace Athena
{
	struct TextureGeneratorData
	{
		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BlackTexture;
		Ref<Texture2D> BRDF_LUT;
		Ref<Texture2D> BlueNoise;
		Ref<TextureCube> BlackTextureCube;
	};

	TextureGeneratorData s_Data;


	void TextureGenerator::Init()
	{
		uint32 whiteTextureData = 0xffffffff;
		Buffer texData = Buffer::Move(&whiteTextureData, sizeof(uint32));

		TextureCreateInfo texInfo;
		texInfo.Name = "Renderer_WhiteTexture";
		texInfo.Format = TextureFormat::RGBA8;
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
		texCubeInfo.Format = TextureFormat::RGBA8;
		texCubeInfo.Usage = TextureUsage(TextureUsage::SAMPLED | TextureUsage::STORAGE);
		texCubeInfo.Width = 1;
		texCubeInfo.Height = 1;
		texCubeInfo.GenerateMipMap = false;
		texCubeInfo.Sampler.Filter = TextureFilter::NEAREST;
		texCubeInfo.Sampler.Wrap = TextureWrap::REPEAT;

		s_Data.BlackTextureCube = TextureCube::Create(texCubeInfo, texData);

		// BLUE NOISE
		{
			const FilePath& resourcesPath = Application::Get().GetConfig().EngineResourcesPath;
			FilePath path = resourcesPath / "Textures/BlueNoise.png";

			TextureImportOptions options;
			options.Name = "Renderer_BlueNoise";
			options.sRGB = false;
			options.GenerateMipMaps = false;

			s_Data.BlueNoise = TextureImporter::Load(path, options);
		}

		// BRDF_LUT
		{
			texInfo.Name = "Renderer_BRDF_LUT";
			texInfo.Format = TextureFormat::RG16F;
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
			commandBuffer->Submit();
		}
	}

	void TextureGenerator::Shutdown()
	{
		s_Data.WhiteTexture.Release();
		s_Data.BlackTexture.Release();
		s_Data.BlueNoise.Release();
		s_Data.BRDF_LUT.Release();
		s_Data.BlackTextureCube.Release();
	}

	Ref<Texture2D> TextureGenerator::GetBRDF_LUT()
	{
		return s_Data.BRDF_LUT;
	}

	Ref<Texture2D> TextureGenerator::GetWhiteTexture()
	{
		return s_Data.WhiteTexture;
	}

	Ref<Texture2D> TextureGenerator::GetBlackTexture()
	{
		return s_Data.BlackTexture;
	}

	Ref<TextureCube> TextureGenerator::GetBlackTextureCube()
	{
		return s_Data.BlackTextureCube;
	}

	Ref<Texture2D> TextureGenerator::GetBlueNoise()
	{
		return s_Data.BlueNoise;
	}
}
