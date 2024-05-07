#include "Font.h"
#include "Athena/Core/Application.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Math/Common.h"

#include <thread>

#include <msdf-atlas-gen/msdf-atlas-gen.h>
#include <msdfgen.h>


namespace Athena
{
    struct FontData
    {
        msdfgen::FreetypeHandle* FTPHandle;
        Ref<Font> DefaultFont;
    };

    static FontData s_Data;

    bool Font::Init()
    {
        s_Data.FTPHandle = msdfgen::initializeFreetype();

        if (!s_Data.FTPHandle)
        {
            ATN_CORE_ERROR_TAG("Renderer", "Failed to initialize freetype!");
            return false;
        }

        FilePath resourcesPath = Application::Get().GetConfig().EngineResourcesPath;
        s_Data.DefaultFont = Font::Create(resourcesPath / "Fonts/Open_Sans/OpenSans-Medium.ttf");

        return true;
    }

    void Font::Shutdown()
    {
        if(s_Data.FTPHandle)
            msdfgen::deinitializeFreetype(s_Data.FTPHandle);

        s_Data.DefaultFont.Release();
    }

    Ref<Font> Font::Create(const FilePath& path)
    {
        Ref<Font> result = Ref<Font>::Create();
        result->m_FilePath = path;

        msdfgen::FontHandle* font = msdfgen::loadFont(s_Data.FTPHandle, path.string().c_str());

        if (font == nullptr)
        {
            ATN_CORE_ERROR_TAG("Renderer", "Failed to load font from {}!", result->m_FilePath);
            return nullptr;
        }

        std::vector<msdf_atlas::GlyphGeometry> glyphs;
        msdf_atlas::FontGeometry fontGeometry(&glyphs);

        fontGeometry.loadCharset(font, 1.0, msdf_atlas::Charset::ASCII);

        msdfgen::destroyFont(font);

        const double maxCornerAngle = 3.0;
        for (msdf_atlas::GlyphGeometry& glyph : glyphs)
            glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);

        msdf_atlas::TightAtlasPacker packer;
        packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
        packer.setMinimumScale(24.0);
        packer.setPixelRange(2.0);
        packer.setMiterLimit(1.0);

        packer.pack(glyphs.data(), glyphs.size());

        int width = 0, height = 0;
        packer.getDimensions(width, height);
        msdf_atlas::ImmediateAtlasGenerator<float, 3, msdf_atlas::msdfGenerator, msdf_atlas::BitmapAtlasStorage<byte, 3>> generator(width, height);

        // The correct way might be count * 1.5, but we dont want to load all cores
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

        TextureCreateInfo atlasInfo;
        atlasInfo.Name = fmt::format("{}_FontAtlas", result->m_FilePath.filename());
        atlasInfo.Format = TextureFormat::RGBA8;
        atlasInfo.Usage = TextureUsage::SAMPLED;
        atlasInfo.Width = width;
        atlasInfo.Height = height;
        atlasInfo.Layers = 1;
        atlasInfo.GenerateMipMap = false;
        atlasInfo.Sampler.Filter = TextureFilter::LINEAR;
        atlasInfo.Sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

        result->m_TextureAtlas = Texture2D::Create(atlasInfo, buffer);
        buffer.Release();

        return result;
    }

    Ref<Font> Font::GetDefault()
    {
        return s_Data.DefaultFont;
    }
}
