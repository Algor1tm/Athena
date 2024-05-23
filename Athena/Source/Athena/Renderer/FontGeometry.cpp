#include "FontGeometry.h"
#include "Athena/Utils/StringUtils.h"


namespace Athena
{
	FontGeometry::FontGeometry(msdfgen::FontHandle* font, const std::vector<CharsetRange>& charsetRanges)
	{
		m_FontGeometry = msdf_atlas::FontGeometry(&m_Glyphs);

		msdf_atlas::Charset charset;
		for (CharsetRange range : charsetRanges)
		{
			for (uint32 c = range.Begin; c <= range.End; ++c)
				charset.add(c);
		}

		m_FontGeometry.loadCharset(font, 1.0, charset);

		const auto& metrics = m_FontGeometry.getMetrics();

		m_Scale = 1.0 / (metrics.ascenderY - metrics.descenderY);;
		m_SpaceGlyphAdvance = m_FontGeometry.getGlyph(' ')->getAdvance();
		m_NewLineAdvance = m_Scale * metrics.lineHeight;

		m_ProcessedIndex = 0;
		m_CoordX = 0.0;
		m_CoordY = m_NewLineAdvance;
		m_TexelSize = { 1.f / 1024.f, 1.f / 1024.f };
	}

	std::vector<msdf_atlas::GlyphGeometry>& FontGeometry::GetGlyphs()
	{
		return m_Glyphs;
	}

	void FontGeometry::SetTexelSize(Vector2 size)
	{
		m_TexelSize = size;
	}

	float FontGeometry::InitText(const std::string& text, TextAlignmentOptions options)
	{
		m_Options = options;
		m_ProcessedString = Utils::ToUTF32String(text);

		const auto& metrics = m_FontGeometry.getMetrics();

		m_NewLineAdvance = -(m_Scale * metrics.lineHeight + options.LineSpacing);
		if (options.InvertY)
			m_NewLineAdvance = -m_NewLineAdvance;

		double x = 0.0;
		double y = m_NewLineAdvance;

		uint32 wordBeginIndex = 0;
		double wordBeginX = 0.0;
		bool insideWord = false;

		for (int64 i = 0; i < m_ProcessedString.size(); ++i)
		{
			char32_t character = m_ProcessedString[i];

			if (character == '\0')
				break;

			if (character == '\r')
				continue;

			if (character == '\n')
			{
				x = 0;
				y += m_NewLineAdvance;
				insideWord = false;
				continue;
			}

			if (character == '\t')
			{
				float advance = 4 * m_SpaceGlyphAdvance;
				double beginX = x;

				x += m_Scale * advance + m_Options.Kerning;
				insideWord = false;

				if (x > m_Options.MaxWidth && beginX != 0.0)
				{
					x = 0;
					y += m_NewLineAdvance;
					m_ProcessedString.insert(i, U"\n");
				}

				continue;
			}

			if (character == ' ')
			{
				float advance;

				if (i < m_ProcessedString.size() - 1)
				{
					char32_t nextCharacter = m_ProcessedString[i + 1];
					if (IsGlyph(nextCharacter))
					{
						double dAdvance;
						m_FontGeometry.getAdvance(dAdvance, character, nextCharacter);
						advance = (float)dAdvance;
					}
					else
					{
						advance = m_SpaceGlyphAdvance;
					}
				}
				else
				{
					advance = m_SpaceGlyphAdvance;
				}

				double beginX = x;

				x += m_Scale * advance + m_Options.Kerning;
				insideWord = false;

				if (x > m_Options.MaxWidth && beginX != 0.0)
				{
					x = 0;
					y += m_NewLineAdvance;
					m_ProcessedString.insert(i, U"\n");
				}

				continue;
			}


			const msdf_atlas::GlyphGeometry* glyph = m_FontGeometry.getGlyph(character);

			if (!glyph)
				glyph = m_FontGeometry.getGlyph('?');

			if (!glyph)
				continue;

			if (insideWord == false)
			{
				insideWord = true;
				wordBeginIndex = i;
				wordBeginX = x;
			}

			if (i < m_ProcessedString.size() - 1)
			{
				double advance = glyph->getAdvance();
				char32_t nextCharacter = m_ProcessedString[i + 1];
				m_FontGeometry.getAdvance(advance, character, nextCharacter);

				x += m_Scale * advance + m_Options.Kerning;
			}

			if (x > m_Options.MaxWidth && wordBeginX != 0.0)
			{
				m_ProcessedString.insert(wordBeginIndex, U"\n");
				i = wordBeginIndex + 1;
				insideWord = false;

				x = 0;
				y += m_NewLineAdvance;
				continue;
			}
		}

		m_ProcessedIndex = 0;

		m_CoordX = 0.0;
		m_CoordY = m_NewLineAdvance;

		return y + m_NewLineAdvance / 2.f ;
	}

	bool FontGeometry::NextCharacter(GlyphData* outGlyphData)
	{
		for (uint32 i = m_ProcessedIndex; i < m_ProcessedString.size(); ++i)
		{
			char32_t character = m_ProcessedString[i];

			if (character == '\0')
				break;

			if (character == '\r')
			{
				m_ProcessedIndex++;
				continue;
			}

			if (character == '\n')
			{
				m_CoordX = 0;
				m_CoordY += m_NewLineAdvance;
				m_ProcessedIndex++;
				continue;
			}

			if (character == '\t')
			{
				float advance = 4 * m_SpaceGlyphAdvance;
				m_CoordX += m_Scale * advance + m_Options.Kerning;
				m_ProcessedIndex++;
				continue;
			}

			if (character == ' ')
			{
				float advance;

				if (i < m_ProcessedString.size() - 1)
				{
					char32_t nextCharacter = m_ProcessedString[i + 1];
					if (IsGlyph(nextCharacter))
					{
						double dAdvance;
						m_FontGeometry.getAdvance(dAdvance, character, nextCharacter);
						advance = (float)dAdvance;
					}
					else
					{
						advance = m_SpaceGlyphAdvance;
					}
				}
				else
				{
					advance = m_SpaceGlyphAdvance;
				}

				m_CoordX += m_Scale * advance + m_Options.Kerning;
				m_ProcessedIndex++;
				continue;
			}

			const msdf_atlas::GlyphGeometry* glyph = m_FontGeometry.getGlyph(character);

			if (!glyph)
			{
				glyph = m_FontGeometry.getGlyph('?');
			}

			if (!glyph)
			{
				m_ProcessedIndex++;
				continue;
			}

			double al, ab, ar, at;
			glyph->getQuadAtlasBounds(al, ab, ar, at);
			Vector2 texCoordMin((float)al, (float)ab);
			Vector2 texCoordMax((float)ar, (float)at);

			texCoordMin *= m_TexelSize;
			texCoordMax *= m_TexelSize;

			double pl, pb, pr, pt;
			glyph->getQuadPlaneBounds(pl, pb, pr, pt);
			Vector2 quadMin((float)pl, (float)pb);
			Vector2 quadMax((float)pr, (float)pt);

			if (m_Options.InvertY)
			{
				quadMin.y = -quadMin.y;
				quadMax.y = -quadMax.y;
			}

			quadMin *= m_Scale, quadMax *= m_Scale;
			quadMin += Vector2(m_CoordX, m_CoordY);
			quadMax += Vector2(m_CoordX, m_CoordY);

			GlyphData glyphData;
			glyphData.TexCoordMin = texCoordMin;
			glyphData.TexCoordMax = texCoordMax;
			glyphData.QuadMin = quadMin;
			glyphData.QuadMax = quadMax;
			glyphData.LocalCoords = { m_CoordX, m_CoordY };

			*outGlyphData = glyphData;

			if (i < m_ProcessedString.size() - 1)
			{
				double advance = glyph->getAdvance();
				char32_t nextCharacter = m_ProcessedString[i + 1];
				m_FontGeometry.getAdvance(advance, character, nextCharacter);

				m_CoordX += m_Scale * advance + m_Options.Kerning;
			}

			m_ProcessedIndex++;
			return true;
		}

		return false;
	}

	bool FontGeometry::IsGlyph(char32_t codepoint)
	{
		bool result = true;
		result = result && codepoint != '\0';
		result = result && codepoint != '\r';
		result = result && codepoint != '\t';
		result = result && codepoint != '\n';

		return result;
	}
}
