#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"

#include <vector>
#include <msdf-atlas-gen/msdf-atlas-gen.h>


namespace Athena
{
	struct CharsetRange
	{
		uint32 Begin, End;
	};

	struct TextAlignmentOptions
	{
		float MaxWidth = 0.0f;
		float Kerning = 0.0f;
		float LineSpacing = 0.0f;
		bool InvertY = false;
	};

	struct GlyphData
	{
		Vector2 TexCoordMin;
		Vector2 TexCoordMax;
		Vector2 QuadMin;
		Vector2 QuadMax;
		Vector2 LocalCoords;
	};

	class ATHENA_API FontGeometry
	{
	public:
		FontGeometry() = default;
		FontGeometry(msdfgen::FontHandle* font, const std::vector<CharsetRange>& charsetRanges);

		std::vector<msdf_atlas::GlyphGeometry>& GetGlyphs();

		void SetTexelSize(Vector2 size);

		float InitText(const std::string& text, TextAlignmentOptions options);
		bool NextCharacter(GlyphData* glyphData);

	private:
		bool IsGlyph(char32_t codepoint);

	private:
		std::vector<msdf_atlas::GlyphGeometry> m_Glyphs;
		msdf_atlas::FontGeometry m_FontGeometry;
		Vector2 m_TexelSize;

		float m_Scale;
		float m_SpaceGlyphAdvance;
		float m_NewLineAdvance;

		TextAlignmentOptions m_Options;
		std::u32string m_ProcessedString;
		int64 m_ProcessedIndex;
		float m_CoordX, m_CoordY;
	};
}
