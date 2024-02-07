#include "Image.h"

#include "Athena/Platform/Vulkan/VulkanImage.h"
#include "Athena/Renderer/Renderer.h"

#include <stb_image/stb_image.h>


namespace Athena
{
	namespace Utils
	{
		static void* ConvertRGBToRGBA(Vector<byte, 3>* data, uint32 width, uint32 height)
		{
			uint64 newSize = width * height * Image::BytesPerPixel(ImageFormat::RGBA8);
			Vector<byte, 4>* newData = (Vector<byte, 4>*)malloc(newSize);

			for (uint64 i = 0; i < width * height; ++i)
			{
				newData[i][0] = data[i][0];
				newData[i][1] = data[i][1];
				newData[i][2] = data[i][2];
				newData[i][3] = 255;
			}

			stbi_image_free(data);
			return newData;
		}

		static void* ConvertRGB32FToRGBA32F(Vector3* data, uint32 width, uint32 height)
		{
			uint64 newSize = width * height * Image::BytesPerPixel(ImageFormat::RGBA32F);
			Vector4* newData = (Vector4*)malloc(newSize);

			for (uint64 i = 0; i < width * height; ++i)
			{
				newData[i][0] = data[i][0];
				newData[i][1] = data[i][1];
				newData[i][2] = data[i][2];
				newData[i][3] = 1.f;
			}

			stbi_image_free(data);
			return newData;
		}
	}


	Ref<Image> Image::Create(const FilePath& filepath)
	{
		int width, height, channels;
		bool HDR = false;
		ImageFormat format = ImageFormat::RGBA8;
		void* data = nullptr;

		String path = filepath.string();
		if (stbi_is_hdr(path.data()))
		{
			data = stbi_loadf(path.data(), &width, &height, &channels, 0);
			HDR = true;
			switch (channels)
			{
			case 3: format = ImageFormat::RGB32F; break;
			case 4: format = ImageFormat::RGBA32F; break;
			default:
				ATN_CORE_ERROR("Failed to load image from {}, width = {}, height = {}, channels = {}", filepath, width, height, channels);
				return nullptr;
			}
		}
		else
		{
			data = stbi_load(path.data(), &width, &height, &channels, 0);
			switch (channels)
			{
			case 3: format = ImageFormat::RGB8_SRGB; break;
			case 4: format = ImageFormat::RGBA8_SRGB; break;
			default:
				ATN_CORE_ERROR("Failed to load image from {}, width = {}, height = {}, channels = {}", filepath, width, height, channels);
				return nullptr;
			}
		}
		ATN_CORE_ASSERT(data);

		if (format == ImageFormat::RGB8_SRGB)
		{
			data = Utils::ConvertRGBToRGBA((Vector<byte, 3>*)data, width, height);
			format = ImageFormat::RGBA8_SRGB;
		}
		else if (format == ImageFormat::RGB32F)
		{
			data = Utils::ConvertRGB32FToRGBA32F((Vector3*)data, width, height);
			format = ImageFormat::RGBA32F;
		}

		ImageCreateInfo info;
		info.Name = filepath.filename().string();
		info.Format = format;
		info.Usage = ImageUsage::DEFAULT;
		info.Type = ImageType::IMAGE_2D;
		info.InitialData = data;
		info.Width = width;
		info.Height = height;
		info.Layers = 1;
		info.MipLevels = 1;

		Ref<Image> result = Image::Create(info);

		stbi_image_free(data);
		return result;
	}

	Ref<Image> Image::Create(const String& name, const void* inputData, uint32 inputWidth, uint32 inputHeight)
	{
		int width, height, channels;
		void* data = nullptr;

		const uint32 size = inputHeight == 0 ? inputWidth : inputWidth * inputHeight;
		data = stbi_load_from_memory((const stbi_uc*)inputData, size, &width, &height, &channels, 0);
		ATN_CORE_ASSERT(data);

		ImageFormat format = ImageFormat::RGBA8_SRGB;

		switch (channels)
		{
		case 3: format = ImageFormat::RGB8_SRGB; break;
		case 4: format = ImageFormat::RGBA8_SRGB; break;
		default:
			ATN_CORE_ERROR("Failed to load image from memory, width = {}, height = {}, channels = {}", width, height, channels);
			return nullptr;
		}

		if (format == ImageFormat::RGB8_SRGB)
		{
			data = Utils::ConvertRGBToRGBA((Vector<byte, 3>*)data, width, height);
			format = ImageFormat::RGBA8_SRGB;
		}

		ImageCreateInfo info;
		info.Name = name;
		info.Format = format;
		info.Usage = ImageUsage::DEFAULT;
		info.Type = ImageType::IMAGE_2D;
		info.InitialData = data;
		info.Width = width;
		info.Height = height;
		info.Layers = 1;
		info.MipLevels = 1;

		Ref<Image> result = Image::Create(info);
		stbi_image_free(data);
		return result;
	}


	Ref<Image> Image::Create(const ImageCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanImage>::Create(info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
