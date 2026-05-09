#include "FontImporter.h"
#include "Athena/Core/Application.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Math/Common.h"
#include "Athena/Renderer/FontGeometry.h"

#include <thread>

#include <msdf-atlas-gen/msdf-atlas-gen.h>
#include <msdfgen.h>


namespace Athena
{
	bool FontImporter::Import(WeakRef<Font> fontAsset, const FilePath& path)
	{
        msdfgen::FontHandle* font = msdfgen::loadFont((msdfgen::FreetypeHandle*)Font::GetFTPHandle(), path.string().c_str());

        if (font == nullptr)
        {
            ATN_CORE_ERROR_TAG("AssetManager", "Failed to load font from {}!", path);
            return false;
        }

        // From imgui_draw.cpp
        const std::vector<CharsetRange> charsetRanges =
        {
            { 0x0020, 0x00FF }, // Basic Latin + Latin Supplement
            { 0x0400, 0x052F }, // Cyrillic + Cyrillic Supplement
            { 0x2DE0, 0x2DFF }, // Cyrillic Extended-A
            { 0xA640, 0xA69F }, // Cyrillic Extended-B
        };

        fontAsset->m_FontGeometry = new FontGeometry(font, charsetRanges);
        std::vector<msdf_atlas::GlyphGeometry>& glyphs = fontAsset->m_FontGeometry->GetGlyphs();

        const double maxCornerAngle = 3.0;
        for (msdf_atlas::GlyphGeometry& glyph : glyphs)
            glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);

        msdf_atlas::TightAtlasPacker packer;
        //packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
        packer.setPixelRange(2.0);
        packer.setMiterLimit(1.0);
        packer.setScale(40.f);

        uint32 remaining = packer.pack(glyphs.data(), glyphs.size());
        ATN_CORE_ASSERT(remaining == 0);

        int width = 0, height = 0;
        packer.getDimensions(width, height);

        Buffer buffer = GenerateAtlasOrReadFromCache(fontAsset, path, width, height);

        TextureCreateInfo atlasInfo;
        atlasInfo.Name = fmt::format("{}_FontAtlas", path.filename());
        atlasInfo.Format = TextureFormat::RGBA8;
        atlasInfo.Usage = TextureUsage::SAMPLED;
        atlasInfo.Width = width;
        atlasInfo.Height = height;
        atlasInfo.Layers = 1;
        atlasInfo.GenerateMipMap = false;
        atlasInfo.Sampler.Filter = TextureFilter::LINEAR;
        atlasInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

        fontAsset->m_AtlasTexture = Texture2D::Create(atlasInfo, buffer);
        buffer.Release();

        msdfgen::destroyFont(font);

        Vector2 texelSize = { 1.f / width, 1.f / height };
        fontAsset->m_FontGeometry->SetTexelSize(texelSize);

        return true;
	}

	Buffer FontImporter::GenerateAtlasOrReadFromCache(WeakRef<Font> fontAsset, const FilePath& path, uint32 width, uint32 height)
	{
        FilePath cacheDirectory = GetAtlasCacheDirectory();
        FilePath name = path.filename();
        name += ".msdf";

        if (!FileSystem::Exists(cacheDirectory))
            FileSystem::CreateDirectory(cacheDirectory);

        for (const auto& dirEntry : std::filesystem::directory_iterator(cacheDirectory))
        {
            if (!dirEntry.is_directory() && dirEntry.path().filename() == name)
            {
                Buffer memory = FileSystem::ReadFileBinary(dirEntry.path());
                return memory;
            }
        }

        const std::vector<msdf_atlas::GlyphGeometry>& glyphs = fontAsset->m_FontGeometry->GetGlyphs();

        msdf_atlas::ImmediateAtlasGenerator<float, 3, msdf_atlas::msdfGenerator, msdf_atlas::BitmapAtlasStorage<byte, 3>> generator(width, height);

        int threadsCount = std::thread::hardware_concurrency();
        threadsCount = threadsCount * 0.75;
        threadsCount = Math::Max(threadsCount, 1);

        msdf_atlas::GeneratorAttributes attributes;
        generator.setAttributes(attributes);
        generator.setThreadCount(threadsCount);
        generator.generate(glyphs.data(), glyphs.size());

        msdf_atlas::BitmapAtlasStorage<byte, 3> storage = generator.atlasStorage();

        uint32 rgb = 3;
        uint64 rgbSize = width * height * rgb;

        msdfgen::BitmapRef<byte, 3> bitMapRef;
        bitMapRef.pixels = new byte[rgbSize];
        bitMapRef.width = width;
        bitMapRef.height = height;

        storage.get(0, 0, bitMapRef);

        // convert to rgba format
        uint32 rgba = 4;
        uint64 rgbaSize = width * height * rgba;
        Buffer buffer(rgbaSize);

        for (uint64 i = 0, j = 0; i < rgbaSize; i += rgba, j += rgb)
        {
            buffer.Data()[i] = bitMapRef.pixels[j];
            buffer.Data()[i + 1] = bitMapRef.pixels[j + 1];
            buffer.Data()[i + 2] = bitMapRef.pixels[j + 2];
            buffer.Data()[i + 3] = 255;
        }

        delete[] bitMapRef.pixels;

        // Save to cache
        FileSystem::WriteFile(cacheDirectory / name, (const char*)buffer.Data(), buffer.Size());
        ATN_CORE_WARN_TAG("AssetManager", "Cached MSDF Font Atlas {}, ({}, {})", name, width, height);

        return buffer;
	}

	FilePath FontImporter::GetAtlasCacheDirectory()
	{
		return Application::Get().GetConfig().EngineResourcesPath / "Cache/FontAtlases";
	}
}
