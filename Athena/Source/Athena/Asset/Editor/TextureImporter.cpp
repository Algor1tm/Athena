#include "TextureImporter.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Log.h"
#include "Athena/Core/YAMLTypes.h"
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

	Ref<AssetImportSettings> TextureImportSettings::Clone() const
	{
		Ref<TextureImportSettings> cloneSettings = Ref<TextureImportSettings>::Create();

		cloneSettings->Name = Name;
		cloneSettings->sRGB = sRGB;
		cloneSettings->GenerateMipMaps = GenerateMipMaps;
		cloneSettings->WrapMode = WrapMode;
		cloneSettings->FilterMode = FilterMode;
		cloneSettings->AnisotropyLevel = AnisotropyLevel;
		cloneSettings->ComputeUsage = ComputeUsage;
		cloneSettings->ExtractChannelsNum = ExtractChannelsNum;

		return cloneSettings;
	}

	bool TextureImportSettings::Serialize(const FilePath& absolutePath) const
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "TextureImportSettings" << YAML::Value << YAML::BeginMap;

		out << YAML::Key << "sRGB" << YAML::Value << sRGB;
		out << YAML::Key << "GenerateMipMaps" << YAML::Value << GenerateMipMaps;
		out << YAML::Key << "WrapMode" << YAML::Value << EnumUtils::TextureWrapToString(WrapMode);
		out << YAML::Key << "FilterMode" << YAML::Value << EnumUtils::TextureFilterToString(FilterMode);
		out << YAML::Key << "AnisotropyLevel" << YAML::Value << AnisotropyLevel;
		out << YAML::Key << "ComputeUsage" << YAML::Value << ComputeUsage;
		out << YAML::Key << "ExtractChannelsNum" << YAML::Value << ExtractChannelsNum;

		out << YAML::EndMap;
		out << YAML::EndMap;

		std::ofstream fout(absolutePath);
		fout << out.c_str();

		return true;
	}

	bool TextureImportSettings::Deserialize(const FilePath& absolutePath)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(absolutePath.string());

			auto root = data["TextureImportSettings"];

			sRGB = root["sRGB"].as<bool>();
			GenerateMipMaps = root["GenerateMipMaps"].as<bool>();
			WrapMode = EnumUtils::TextureWrapFromString(root["WrapMode"].as<String>());
			FilterMode = EnumUtils::TextureFilterFromString(root["FilterMode"].as<String>());
			AnisotropyLevel = root["AnisotropyLevel"].as<float>();
			ComputeUsage = root["ComputeUsage"].as<bool>();
			ExtractChannelsNum = root["ExtractChannelsNum"].as<uint32>();

			AnisotropyLevel = Math::Clamp(AnisotropyLevel, 0.f, 16.f);
			ExtractChannelsNum = Math::Clamp(ExtractChannelsNum, 1, 4);

			if (ExtractChannelsNum == 3)
				ExtractChannelsNum = 4;
		}
		catch (YAML::Exception& e)
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to load texture import settings from {}. Error message:\n {}", absolutePath, e.what());
			return false;
		}

		return true;
	}

	TextureImporter::TextureImporter(const Ref<TextureImportSettings>& settings)
	{
		m_Settings = settings;
	}

	Ref<Texture2D> TextureImporter::Import(const FilePath& filepath)
	{
		uint32 maxChannels = m_Settings->ExtractChannelsNum;
		if (maxChannels == 3) // image tiling optimal
			maxChannels = 4;

		int width, height, channels;
		Format format = Format::NONE;
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
			format = GetFormat(channels, m_Settings->sRGB);
		}

		if (data == nullptr || format == Format::NONE)
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to import texture from{}, (width = {}, height = {}, channels = {})", filepath, width, height, channels);
			return nullptr;
		}

		bool extract = channels == 3 || maxChannels < channels;
		if (HDR == false && extract)
		{
			data = ExtractChannels((byte*)data, width, height, channels, maxChannels);
			format = GetFormat(maxChannels, m_Settings->sRGB);
		}
		else if (extract)
		{
			data = ExtractChannelsHDR((float*)data, width, height, channels, maxChannels);
			format = GetHDRFormat(maxChannels);
		}

		uint64 size = width * height * FormatUtils::BytesPerPixel(format);
		Buffer buffer = Buffer::Move(data, size);

		TextureUsage usage = TextureUsage::SAMPLED;
		if (m_Settings->ComputeUsage)
			usage = TextureUsage(usage | TextureUsage::STORAGE);

		TextureSamplerCreateInfo samplerInfo;
		samplerInfo.Filter = m_Settings->FilterMode;
		samplerInfo.Wrap = m_Settings->WrapMode;
		samplerInfo.AnisotropyLevel = m_Settings->AnisotropyLevel;
		samplerInfo.Compare = TextureCompareOperator::NONE;

		TextureCreateInfo info;
		info.Name = m_Settings->Name.empty() ? filepath.filename().string() : m_Settings->Name;
		info.TextureFormat = format;
		info.Usage = usage;
		info.Width = width;
		info.Height = height;
		info.Layers = 1;
		info.GenerateMipMap = m_Settings->GenerateMipMaps;
		info.Sampler = samplerInfo;

		Ref<Texture2D> result = Texture2D::Create(info, buffer);
		result->m_FilePath = filepath;

		buffer.Release();
		return result;
	}

	Ref<Texture2D> TextureImporter::ImportFromMemory(const void* inputData, uint32 inputWidth, uint32 inputHeight)
	{
		uint32 maxChannels = m_Settings->ExtractChannelsNum;
		if (maxChannels == 3) // image tiling optimal
			maxChannels = 4;

		int width, height, channels;
		void* data = nullptr;

		const uint32 size = inputHeight == 0 ? inputWidth : inputWidth * inputHeight;
		data = stbi_load_from_memory((const stbi_uc*)inputData, size, &width, &height, &channels, 0);

		Format format = GetFormat(channels, m_Settings->sRGB);

		if (data == nullptr || format == Format::NONE)
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to import texture from memory, (name = {}, width = {}, height = {}, channels = {})", m_Settings->Name, width, height, channels);
			return nullptr;
		}

		if (channels == 3 || maxChannels < channels)
		{
			data = ExtractChannels((byte*)data, width, height, channels, maxChannels);
			format = GetFormat(maxChannels, m_Settings->sRGB);
		}

		uint64 dataSize = width * height * FormatUtils::BytesPerPixel(format);
		Buffer buffer = Buffer::Move(data, dataSize);

		TextureUsage usage = TextureUsage::SAMPLED;
		if (m_Settings->ComputeUsage)
			usage = TextureUsage(usage | TextureUsage::STORAGE);

		TextureSamplerCreateInfo samplerInfo;
		samplerInfo.Filter = m_Settings->FilterMode;
		samplerInfo.Wrap = m_Settings->WrapMode;
		samplerInfo.AnisotropyLevel = m_Settings->AnisotropyLevel;
		samplerInfo.Compare = TextureCompareOperator::NONE;

		TextureCreateInfo info;
		info.Name = m_Settings->Name;
		info.TextureFormat = format;
		info.Usage = usage;
		info.Width = width;
		info.Height = height;
		info.Layers = 1;
		info.GenerateMipMap = m_Settings->GenerateMipMaps;
		info.Sampler = samplerInfo;

		Ref<Texture2D> result = Texture2D::Create(info, buffer);
		buffer.Release();

		return result;
	}

	Format TextureImporter::GetFormat(uint32 channels, bool sRGB)
	{
		switch (channels)
		{
		case 1: return sRGB ? Format::R8_SRGB    : Format::R8;
		case 2: return sRGB ? Format::RG8_SRGB   : Format::RG8;
		case 3: return sRGB ? Format::RGB8_SRGB  : Format::RGB8;
		case 4: return sRGB ? Format::RGBA8_SRGB : Format::RGBA8;
		}

		ATN_CORE_ASSERT(false);
		return Format::NONE;
	}

	Format TextureImporter::GetHDRFormat(uint32 channels)
	{
		switch (channels)
		{
		case 3: return Format::RGB32F;
		case 4: return Format::RGBA32F;
		}

		ATN_CORE_ASSERT(false);
		return Format::NONE;
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

	void TextureExporter::ExportAsPNG(const FilePath& path, const Ref<Texture2D>& texture)
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
		uint32 channels = FormatUtils::ChannelsNum(texture->GetFormat());
		uint32 bpp = FormatUtils::BytesPerPixel(texture->GetFormat());

		bool result = stbi_write_png(utf8Path.data(), width, height, channels, buffer.Data(), bpp * width);

		buffer.Release();

		if (!result)
			ATN_CORE_ERROR_TAG("Asset", "Failed to save texture in file '{}'", path);
	}
}
