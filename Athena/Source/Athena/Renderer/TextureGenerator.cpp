#include "TextureGenerator.h"

#include "Athena/Core/Application.h"
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
		Buffer texData = Buffer::Copy(&whiteTextureData, sizeof(uint32));

		TextureCreateInfo texInfo;
		texInfo.Name = "Renderer_WhiteTexture";
		texInfo.Format = TextureFormat::RGBA8;
		texInfo.Usage = TextureUsage(TextureUsage::SAMPLED | TextureUsage::STORAGE);
		texInfo.Width = 1;
		texInfo.Height = 1;
		texInfo.Layers = 1;
		texInfo.GenerateMipLevels = false;
		texInfo.Sampler.MinFilter = TextureFilter::NEAREST;
		texInfo.Sampler.MagFilter = TextureFilter::NEAREST;
		texInfo.Sampler.MipMapFilter = TextureFilter::NEAREST;
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
		texCubeInfo.GenerateMipLevels = false;
		texCubeInfo.Sampler.MinFilter = TextureFilter::NEAREST;
		texCubeInfo.Sampler.MagFilter = TextureFilter::NEAREST;
		texCubeInfo.Sampler.MipMapFilter = TextureFilter::NEAREST;
		texCubeInfo.Sampler.Wrap = TextureWrap::REPEAT;

		s_Data.BlackTextureCube = TextureCube::Create(texCubeInfo, texData);

		texData.Release();

		// BLUE NOISE
		{
			const FilePath& resourcesPath = Application::Get().GetConfig().EngineResourcesPath;
			FilePath path = resourcesPath / "Textures/BlueNoise.png";

			int width, height, channels;
			Vector<byte, 3>* data = nullptr;

			data = (Vector<byte, 3>*)stbi_load(path.string().data(), &width, &height, &channels, 0);
			ATN_CORE_ASSERT(channels == 3);

			uint64 newSize = width * height * Texture::BytesPerPixel(TextureFormat::R8);
			Vector<byte, 1>* newData = (Vector<byte, 1>*)malloc(newSize);

			for (uint64 i = 0; i < width * height; ++i)
			{
				newData[i][0] = data[i][0];
			}

			stbi_image_free(data);
			texData = Buffer::Copy(newData, width * height * Texture::BytesPerPixel(TextureFormat::R8));

			texInfo.Name = "Renderer_BlueNoise";
			texInfo.Format = TextureFormat::R8;
			texInfo.Usage = TextureUsage(TextureUsage::SAMPLED);
			texInfo.Width = width;
			texInfo.Height = height;
			texInfo.Layers = 1;
			texInfo.GenerateMipLevels = false;
			texInfo.Sampler.MinFilter = TextureFilter::LINEAR;
			texInfo.Sampler.MagFilter = TextureFilter::LINEAR;
			texInfo.Sampler.MipMapFilter = TextureFilter::NEAREST;
			texInfo.Sampler.Wrap = TextureWrap::REPEAT;

			s_Data.BlueNoise = Texture2D::Create(texInfo, texData);

			texData.Release();
		}

		// BRDF_LUT
		{
			texInfo.Name = "Renderer_BRDF_LUT";
			texInfo.Format = TextureFormat::RG16F;
			texInfo.Usage = TextureUsage(TextureUsage::STORAGE | TextureUsage::SAMPLED);
			texInfo.Width = 512;
			texInfo.Height = 512;
			texInfo.Layers = 1;
			texInfo.GenerateMipLevels = false;
			texInfo.Sampler.MinFilter = TextureFilter::LINEAR;
			texInfo.Sampler.MagFilter = TextureFilter::LINEAR;
			texInfo.Sampler.MipMapFilter = TextureFilter::LINEAR;
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
