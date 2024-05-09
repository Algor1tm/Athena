#pragma once

#include "Athena/Core/Core.h"

#include <vector>
#include <msdf-atlas-gen/msdf-atlas-gen.h>


namespace Athena
{
	struct FontAtlasGeometryData
	{
		std::vector<msdf_atlas::GlyphGeometry> Glyphs;
		msdf_atlas::FontGeometry FontGeometry;
	};
}
