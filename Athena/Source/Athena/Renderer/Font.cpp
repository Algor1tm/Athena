#include "Font.h"
#include "Athena/Asset/Editor/FontImporter.h"
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
    Font::FontStaticData Font::s_Data;

    bool Font::Init()
    {
        s_Data.FTPHandle = msdfgen::initializeFreetype();

        if (!s_Data.FTPHandle)
        {
            ATN_CORE_ERROR_TAG("Renderer", "Failed to initialize freetype!");
            return false;
        }

        FilePath resourcesPath = Application::Get().GetConfig().EngineResourcesPath;

        s_Data.DefaultFont = Ref<Font>::Create();

        FontImporter fontImporter;
        fontImporter.Import(s_Data.DefaultFont, resourcesPath / "Fonts/Open_Sans/OpenSans-Medium.ttf");

        return true;
    }

    void Font::Shutdown()
    {
        if(s_Data.FTPHandle)
            msdfgen::deinitializeFreetype((msdfgen::FreetypeHandle*)s_Data.FTPHandle);

        s_Data.DefaultFont.Release();
    }

    Ref<Font> Font::GetDefault()
    {
        return s_Data.DefaultFont;
    }

    void* Font::GetFTPHandle()
    {
        return s_Data.FTPHandle;
    }

    Font::Font()
        : m_FontGeometry(nullptr)
    {

    }

    Font::~Font()
    {
        delete m_FontGeometry;
    }

    bool Font::Serialize(const FilePath& absolutePath) const
    {
        // Do nothing
        return true;
    }

    bool Font::Deserialize(const FilePath& absolutePath, Ref<AssetImportSettings> importSettings)
    {
        FontImporter fontImporter;
        bool result = fontImporter.Import(this, absolutePath);

        return result;
    }
}
