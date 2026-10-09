#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Font.h"


namespace Athena
{
	class ATHENA_API FontImporter
	{
	public:
		bool Import(WeakRef<Font> fontAsset, const FilePath& path);

	private:
		Buffer GenerateAtlasOrReadFromCache(WeakRef<Font> fontAsset, const FilePath& path, uint32 width, uint32 height);
		FilePath GetAtlasCacheDirectory();
	};
}
