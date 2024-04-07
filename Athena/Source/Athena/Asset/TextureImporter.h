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

		TextureSamplerCreateInfo Sampler;
	};

	class ATHENA_API TextureImporter
	{
	public:
		static Ref<Texture2D> Load(const FilePath& path, bool sRGB = false);
		static Ref<Texture2D> Load(const FilePath& path, const TextureImportOptions& options = TextureImportOptions());

		static Ref<Texture2D> Load(const void* data, uint32 width, uint32 height, const TextureImportOptions& options = TextureImportOptions());

	private:
		static TextureFormat GetFormat(uint32 channels, bool sRGB);
		static TextureFormat GetHDRFormat(uint32 channels);

		static void* ConvertRGBToRGBA(Vector<byte, 3>* data, uint32 width, uint32 height);
		static void* ConvertRGB32FToRGBA32F(Vector3* data, uint32 width, uint32 height);
	};


	class ATHENA_API TextureExporter
	{
	public:
		static void ExportPNG(const FilePath& path, const Ref<Texture2D>& texture);
	};
}
