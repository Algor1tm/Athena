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
		ATN_CORE_VERIFY(options.MaxChannelsNum != 0 && options.MaxChannelsNum <= 4);

		uint32 maxChannels = options.MaxChannelsNum;
		if (maxChannels == 3) // image tiling optimal
			maxChannels = 4;

		int width, height, channels;
		TextureFormat format = TextureFormat::NONE;
		void* data = nullptr;
		bool HDR = false;

		auto utf8Path = Utils::ConvertPathToUTF8(filepath);
		if (stbi_is_hdr(utf8Path.data()))
		{
			data = stbi_loadf(utf8Path.data(), &width, &height, &channels, 0);
			format = GetHDRFormat(channels);
			HDR = true;
		}
		else
		{
			data = stbi_load(utf8Path.data(), &width, &height, &channels, 0);
			format = GetFormat(channels, options.sRGB);
		}

		if (data == nullptr || format == TextureFormat::NONE)
		{
			ATN_CORE_ERROR("Failed to load image from {}, width = {}, height = {}, channels = {}", filepath, width, height, channels);
			ATN_CORE_VERIFY(false);
			return nullptr;
		}

		bool extract = channels == 3 || maxChannels < channels;
		if (HDR == false && extract)
		{
			data = ExtractChannels((byte*)data, width, height, channels, maxChannels);
			format = GetFormat(maxChannels, options.sRGB);
		}
		else if (extract)
		{
			data = ExtractChannelsHDR((float*)data, width, height, channels, maxChannels);
			format = GetHDRFormat(maxChannels);
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
		info.GenerateMipMap = options.GenerateMipMaps;
		info.Sampler = options.Sampler;

		Ref<Texture2D> result = Texture2D::Create(info, buffer);
		result->m_FilePath = filepath;

		buffer.Release();
		return result;
	}

	Ref<Texture2D> TextureImporter::Load(const void* inputData, uint32 inputWidth, uint32 inputHeight, const TextureImportOptions& options)
	{
		ATN_CORE_VERIFY(options.MaxChannelsNum != 0 && options.MaxChannelsNum <= 4);

		uint32 maxChannels = options.MaxChannelsNum;
		if (maxChannels == 3) // image tiling optimal
			maxChannels = 4;

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

		if (channels == 3 || maxChannels < channels)
		{
			data = ExtractChannels((byte*)data, width, height, channels, maxChannels);
			format = GetFormat(maxChannels, options.sRGB);
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
		info.GenerateMipMap = options.GenerateMipMaps;
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
		case 3: return TextureFormat::RGB32F;
		case 4: return TextureFormat::RGBA32F;
		}

		ATN_CORE_ASSERT(false);
		return TextureFormat::NONE;
	}

	void* TextureImporter::ExtractChannels(byte* data, uint32 width, uint32 height, uint32 channels, uint32 desiredChannels)
	{
		uint64 newSize = width * height * desiredChannels;
		byte* newData = (byte*)malloc(newSize);
		uint32 minChannel = Math::Min(channels, desiredChannels);

		uint64 k = 0;
		for (uint64 i = 0; i < width * height * channels; i += channels)
		{
			for (uint32 j = 0; j < minChannel; ++j)
				newData[k + j] = data[i + j];
			
			for(uint32 j = minChannel; j < desiredChannels; ++j)
				newData[k + j] = 255;

			k += desiredChannels;
		}

		stbi_image_free(data);
		return newData;
	}

	void* TextureImporter::ExtractChannelsHDR(float* data, uint32 width, uint32 height, uint32 channels, uint32 desiredChannels)
	{
		uint64 newSize = width * height * desiredChannels * 4;
		float* newData = (float*)malloc(newSize);
		uint32 minChannel = Math::Min(channels, desiredChannels);

		uint64 k = 0;
		for (uint64 i = 0; i < width * height * channels; i += channels)
		{
			for (uint32 j = 0; j < minChannel; ++j)
				newData[k + j] = data[i + j];

			for (uint32 j = minChannel; j < desiredChannels; ++j)
				newData[k + j] = 1.f;

			k += desiredChannels;
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

		uint32 width = texture->GetWidth();
		uint32 height = texture->GetHeight();

		auto utf8Path = Utils::ConvertPathToUTF8(path);
		uint32 channels = Texture::ChannelsNum(texture->GetFormat());
		uint32 bpp = Texture::BytesPerPixel(texture->GetFormat());

		bool result = stbi_write_png(utf8Path.data(), width, height, channels, buffer.Data(), bpp * width);

		buffer.Release();

		if (!result)
			ATN_CORE_ERROR_TAG("Asset", "Failed to save texture in file '{}'", path);
	}
}
