#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	struct TextureImportOptions
	{
		String Name;
		TextureUsage Usage = TextureUsage::DEFAULT;
		bool sRGB = false;
		bool GenerateMipMaps = true;
		uint32 MaxChannelsNum = 4;	 // 3 is not supported

		TextureSamplerCreateInfo Sampler;
	};

	class ATHENA_API TextureImporter
	{
	public:
		Ref<Texture2D> Import(const FilePath& path);
		Ref<Texture2D> ImportFromMemory(const void* data, uint32 width, uint32 height);

		void SetName(const String& name) { m_Name = name; }
		void SetTextureUsage(TextureUsage usage) { m_TextureUsage = usage; }
		void SetIsSRGB(bool sRGB) { m_SRGB = sRGB; }
		void SetGenererateMipMaps(bool generate) { m_GenerateMipMaps = generate; }
		void SetMaxChannels(uint32 maxChannels) { m_MaxChannels = Math::Clamp(maxChannels, 1, 4); }
		void SetSamplerInfo(const TextureSamplerCreateInfo& samplerInfo) { m_SamplerInfo = samplerInfo; }

	private:
		TextureFormat GetFormat(uint32 channels, bool sRGB);
		TextureFormat GetHDRFormat(uint32 channels);

		void* ExtractChannels(byte* data, uint32 width, uint32 height, uint32 channels, uint32 desiredChannels);
		void* ExtractChannelsHDR(float* data, uint32 width, uint32 height, uint32 channels, uint32 desiredChannels);

	private:
		String m_Name;
		TextureUsage m_TextureUsage = TextureUsage::DEFAULT;
		bool m_SRGB = false;
		bool m_GenerateMipMaps = true;
		uint32 m_MaxChannels = 4;	 // 3 is not supported

		TextureSamplerCreateInfo m_SamplerInfo;
	};


	class ATHENA_API TextureExporter
	{
	public:
		static void ExportAsPNG(const FilePath& path, const Ref<Texture2D>& texture);
	};
}
