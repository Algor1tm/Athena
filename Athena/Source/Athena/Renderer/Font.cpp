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
    bool Font::Init()
    {
        s_FTPHandle = msdfgen::initializeFreetype();

        if (!s_FTPHandle)
        {
            ATN_LOG_FATAL(Renderer, "Failed to initialize freetype!");
            return false;
        }

        return true;
    }

    void Font::Shutdown()
    {
        if(s_FTPHandle)
            msdfgen::deinitializeFreetype((msdfgen::FreetypeHandle*)s_FTPHandle);
    }

    void* Font::GetFTPHandle()
    {
        return s_FTPHandle;
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
