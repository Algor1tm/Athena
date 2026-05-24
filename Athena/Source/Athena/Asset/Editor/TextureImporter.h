#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	class ATHENA_API TextureImportSettings : public AssetImportSettings
	{
	public:
		virtual Ref<AssetImportSettings> Clone() const override;

		virtual bool Serialize(const FilePath& absolutePath) const override;
		virtual bool Deserialize(const FilePath& absolutePath) override;

		String Name;
		bool sRGB = false;
		bool GenerateMipMaps = true;

		TextureWrap WrapMode = TextureWrap::REPEAT;
		TextureFilter FilterMode = TextureFilter::TRILINEAR;
		float AnisotropyLevel = 8.f;

		bool ComputeUsage = false;
		uint32 ExtractChannelsNum = 4;	 // 3 is not supported
	};


	class ATHENA_API TextureImporter
	{
	public:
		TextureImporter(const Ref<TextureImportSettings>& settings);

		Ref<Texture2D> Import(const FilePath& path);
		Ref<Texture2D> ImportFromMemory(const void* data, uint32 width, uint32 height);

	private:
		Format GetFormat(uint32 channels, bool sRGB);
		Format GetHDRFormat(uint32 channels);

		void* ExtractChannels(byte* data, uint32 width, uint32 height, uint32 channels, uint32 desiredChannels);
		void* ExtractChannelsHDR(float* data, uint32 width, uint32 height, uint32 channels, uint32 desiredChannels);

	private:
		Ref<TextureImportSettings> m_Settings;
	};


	class ATHENA_API TextureExporter
	{
	public:
		void ExportAsPNG(const FilePath& path, const Ref<Texture2D>& texture);
	};
}
