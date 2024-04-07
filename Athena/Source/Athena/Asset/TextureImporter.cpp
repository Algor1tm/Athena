#include "TextureImporter.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Log.h"
#include "Athena/Math/Common.h"

#include <stb_image/stb_image.h>
#include <stb_image/stb_image_write.h>



namespace Athena
{
	namespace Utils
	{
		static std::vector<char> ConvertPathToUTF8(const FilePath& path)
		{
			std::vector<char> utf8Path(path.u8string().size() + 1);
			stbiw_convert_wchar_to_utf8(utf8Path.data(), utf8Path.size(), path.c_str());
			return utf8Path;
		}
	}


	Ref<Texture2D> TextureImporter::Load(const FilePath& filepath, bool sRGB)
	{
		TextureImportOptions options;
		options.sRGB = sRGB;

		return Load(filepath, options);
	}

	Ref<Texture2D> TextureImporter::Load(const FilePath& filepath, const TextureImportOptions& options)
	{
		ATN_CORE_VERIFY(FileSystem::Exists(filepath));

		int width, height, channels;
		TextureFormat format = TextureFormat::NONE;
		void* data = nullptr;

		auto utf8Path = Utils::ConvertPathToUTF8(filepath);
		if (stbi_is_hdr(utf8Path.data()))
		{
			data = stbi_loadf(utf8Path.data(), &width, &height, &channels, 0);
			format = GetHDRFormat(channels);
		}
		else
		{
			data = stbi_load(utf8Path.data(), &width, &height, &channels, 0);
			format = GetFormat(channels, options.sRGB);
		}

		if (data == nullptr || format == TextureFormat::NONE)
		{
			ATN_CORE_ERROR("Failed to load image from {}, width = {}, height = {}, channels = {}", filepath, width, height, channels);
			ATN_CORE_ASSERT(false);
			return nullptr;
		}

		if (format == TextureFormat::RGB8_SRGB || format == TextureFormat::RGB8)
		{
			data = ConvertRGBToRGBA((Vector<byte, 3>*)data, width, height);
			format = options.sRGB ? TextureFormat::RGBA8_SRGB : TextureFormat::RGBA8;
		}
		else if (format == TextureFormat::RGB32F)
		{
			data = ConvertRGB32FToRGBA32F((Vector3*)data, width, height);
			format = TextureFormat::RGBA32F;
		}

		uint64 size = width * height * Texture::BytesPerPixel(format);
		Buffer buffer = Buffer::Move(data, size);

		TextureCreateInfo info;
		info.Name = options.Name.empty() ? filepath.filename().string() : options.Name;
		info.Format = format;
		info.Usage = options.Usage;
		info.Width = width;
		info.Height = height;
		info.Layers = 1;
		info.GenerateMipLevels = options.GenerateMipMaps;
		info.Sampler = options.Sampler;

		Ref<Texture2D> result = Texture2D::Create(info, buffer);
		result->m_FilePath = filepath;

		buffer.Release();
		return result;
	}

	Ref<Texture2D> TextureImporter::Load(const void* inputData, uint32 inputWidth, uint32 inputHeight, const TextureImportOptions& options)
	{
		int width, height, channels;
		void* data = nullptr;

		const uint32 size = inputHeight == 0 ? inputWidth : inputWidth * inputHeight;
		data = stbi_load_from_memory((const stbi_uc*)inputData, size, &width, &height, &channels, 0);

		TextureFormat format = GetFormat(channels, options.sRGB);

		if (data == nullptr || format == TextureFormat::NONE)
		{
			ATN_CORE_ERROR("Failed to load image from memory '{}', width = {}, height = {}, channels = {}", options.Name, width, height, channels);
			ATN_CORE_ASSERT(false);
			return nullptr;
		}

		if (channels == 3)
		{
			data = ConvertRGBToRGBA((Vector<byte, 3>*)data, width, height);
			format = options.sRGB ? TextureFormat::RGBA8_SRGB : TextureFormat::RGBA8;
		}

		uint64 dataSize = width * height * Texture::BytesPerPixel(format);
		Buffer buffer = Buffer::Move(data, dataSize);

		TextureCreateInfo info;
		info.Name = options.Name;
		info.Format = format;
		info.Usage = options.Usage;
		info.Width = width;
		info.Height = height;
		info.Layers = 1;
		info.GenerateMipLevels = options.GenerateMipMaps;
		info.Sampler = options.Sampler;

		Ref<Texture2D> result = Texture2D::Create(info, buffer);
		buffer.Release();
		return result;
	}

	TextureFormat TextureImporter::GetFormat(uint32 channels, bool sRGB)
	{
		switch (channels)
		{
		case 1: return sRGB ? TextureFormat::R8_SRGB    : TextureFormat::R8;
		case 2: return sRGB ? TextureFormat::RG8_SRGB   : TextureFormat::RG8;
		case 3: return sRGB ? TextureFormat::RGB8_SRGB  : TextureFormat::RGB8;
		case 4: return sRGB ? TextureFormat::RGBA8_SRGB : TextureFormat::RGBA8;
		}

		ATN_CORE_ASSERT(false);
		return TextureFormat::NONE;
	}

	TextureFormat TextureImporter::GetHDRFormat(uint32 channels)
	{
		switch (channels)
		{
		case 2: return TextureFormat::RG16F;
		case 3: return TextureFormat::RGB32F;
		case 4: return TextureFormat::RGBA32F;
		}

		ATN_CORE_ASSERT(false);
		return TextureFormat::NONE;
	}

	void* TextureImporter::ConvertRGBToRGBA(Vector<byte, 3>* data, uint32 width, uint32 height)
	{
		uint64 newSize = width * height * Texture::BytesPerPixel(TextureFormat::RGBA8);
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

	void* TextureImporter::ConvertRGB32FToRGBA32F(Vector3 * data, uint32 width, uint32 height)
	{
		uint64 newSize = width * height * Texture::BytesPerPixel(TextureFormat::RGBA32F);
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

	void TextureExporter::ExportPNG(const FilePath& path, const Ref<Texture2D>& texture)
	{
		Buffer buffer;

		texture->WriteContentToBuffer(&buffer);

		if (buffer.Size() == 0)
		{
			ATN_CORE_ERROR_TAG("Asset", "Failed to save texture in file '{}' (failed to read texture memory)", path);
			ATN_CORE_ASSERT(false);
			return;
		}

		//auto utf8Path = Utils::ConvertPathToUTF8(path);
		String utf8Path = path.string();
		uint32 bpp = Texture::BytesPerPixel(texture->GetFormat());

		bool result = stbi_write_png(utf8Path.data(), texture->GetWidth(), texture->GetHeight(), bpp, buffer.Data(), bpp * texture->GetWidth());

		buffer.Release();

		if (!result)
			ATN_CORE_ERROR_TAG("Asset", "Failed to save texture in file '{}'", path);
	}
}
