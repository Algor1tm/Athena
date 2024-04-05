#include "Image.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"

#include <stb_image/stb_image.h>
#include <stb_image/stb_image_write.h>


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

		static std::vector<char> ConvertPathToUTF8(const FilePath& path)
		{
			std::vector<char> utf8Path(path.u8string().size() + 1);
			stbiw_convert_wchar_to_utf8(utf8Path.data(), utf8Path.size(), path.c_str());
			return utf8Path;
		}
	}


	Ref<Image> Image::Create(const FilePath& filepath, bool sRGB, bool genMips)
	{
		ATN_CORE_VERIFY(FileSystem::Exists(filepath));

		int width, height, channels;
		bool HDR = false;
		ImageFormat format = ImageFormat::RGBA8;
		void* data = nullptr;

		auto utf8Path = Utils::ConvertPathToUTF8(filepath);
		if (stbi_is_hdr(utf8Path.data()))
		{
			data = stbi_loadf(utf8Path.data(), &width, &height, &channels, 0);
			HDR = true;
			switch (channels)
			{
			case 3: format = ImageFormat::RGB32F; break;
			case 4: format = ImageFormat::RGBA32F; break;
			default:
				ATN_CORE_ERROR("Failed to load image from {}, width = {}, height = {}, channels = {}", filepath, width, height, channels);
				ATN_CORE_ASSERT(false);
				return nullptr;
			}
		}
		else
		{
			data = stbi_load(utf8Path.data(), &width, &height, &channels, 0);
			switch (channels)
			{
			case 1: format = sRGB ? ImageFormat::R8_SRGB    : ImageFormat::R8;	  break;
			case 2: format = sRGB ? ImageFormat::RG8_SRGB   : ImageFormat::RG8;	  break;
			case 3: format = sRGB ? ImageFormat::RGB8_SRGB  : ImageFormat::RGB8;  break;
			case 4: format = sRGB ? ImageFormat::RGBA8_SRGB : ImageFormat::RGBA8; break;
			default:
				ATN_CORE_ERROR("Failed to load image from {}, width = {}, height = {}, channels = {}", filepath, width, height, channels);
				ATN_CORE_ASSERT(false);
				return nullptr;
			}
		}
		ATN_CORE_ASSERT(data);

		if (format == ImageFormat::RGB8_SRGB || format == ImageFormat::RGB8)
		{
			data = Utils::ConvertRGBToRGBA((Vector<byte, 3>*)data, width, height);
			format = sRGB ? ImageFormat::RGBA8_SRGB : ImageFormat::RGBA8;
		}
		else if (format == ImageFormat::RGB32F)
		{
			data = Utils::ConvertRGB32FToRGBA32F((Vector3*)data, width, height);
			format = ImageFormat::RGBA32F;
		}

		Buffer buffer = Buffer::Copy(data, width * height * Image::BytesPerPixel(format));

		ImageCreateInfo info;
		info.Name = filepath.filename().string();
		info.Format = format;
		info.Usage = ImageUsage::DEFAULT;
		info.Type = ImageType::IMAGE_2D;
		info.Width = width;
		info.Height = height;
		info.Layers = 1;
		info.GenerateMipLevels = genMips;

		Ref<Image> result = Image::Create(info, buffer);

		stbi_image_free(data);
		buffer.Release();

		return result;
	}

	Ref<Image> Image::Create(const String& name, const void* inputData, uint32 inputWidth, uint32 inputHeight, bool sRGB, bool genMips)
	{
		int width, height, channels;
		void* data = nullptr;

		const uint32 size = inputHeight == 0 ? inputWidth : inputWidth * inputHeight;
		data = stbi_load_from_memory((const stbi_uc*)inputData, size, &width, &height, &channels, 0);
		ATN_CORE_ASSERT(data);

		ImageFormat format = ImageFormat::RGBA8_SRGB;

		switch (channels)
		{
		case 1: format = sRGB ? ImageFormat::R8_SRGB    : ImageFormat::R8;    break;
		case 2: format = sRGB ? ImageFormat::RG8_SRGB   : ImageFormat::RG8;   break;
		case 3: format = sRGB ? ImageFormat::RGB8_SRGB  : ImageFormat::RGB8;  break;
		case 4: format = sRGB ? ImageFormat::RGBA8_SRGB : ImageFormat::RGBA8; break;
		default:
			ATN_CORE_ERROR("Failed to load image from memory, width = {}, height = {}, channels = {}", width, height, channels);
			ATN_CORE_ASSERT(false);
			return nullptr;
		}

		if (channels == 3)
		{
			data = Utils::ConvertRGBToRGBA((Vector<byte, 3>*)data, width, height);
			format = sRGB ? ImageFormat::RGBA8_SRGB : ImageFormat::RGBA8;
		}

		Buffer buffer = Buffer::Copy(data, width * height * Image::BytesPerPixel(format));

		ImageCreateInfo info;
		info.Name = name;
		info.Format = format;
		info.Usage = ImageUsage::DEFAULT;
		info.Type = ImageType::IMAGE_2D;
		info.Width = width;
		info.Height = height;
		info.Layers = 1;
		info.GenerateMipLevels = genMips;

		Ref<Image> result = Image::Create(info, buffer);

		stbi_image_free(data);
		buffer.Release();

		return result;
	}

	void Image::SaveContentToFile(const FilePath& path, Buffer imageData)
	{
		auto utf8Path = Utils::ConvertPathToUTF8(path);
		uint32 bpp = Image::BytesPerPixel(m_Info.Format);
		stbi_write_png(utf8Path.data(), m_Info.Width, m_Info.Height, bpp, imageData.Data(), bpp * m_Info.Width);
	}

	Ref<Image> Image::Create(const ImageCreateInfo& info, Buffer data)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanImage>::Create(info, data);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
