#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Common.h"

#define UNICODE_CODEPOINT_MAX 0xFFFF
#define UNICODE_CODEPOINT_INVALID 0xFFFD

namespace Athena::Utils
{
	inline String MemoryBytesToString(uint64 bytes)
	{
		// MBs
		if (bytes > 1024 * 1024)
			return std::format("{:.2f} MBs", (float)bytes / (1024.f * 1024.f));

		// KBs
		if (bytes > 1024)
			return std::format("{:.2f} KBs", (float)bytes / 1024.f);

		return std::format("{} bytes", bytes);
	}

	// From imgui.cpp
	inline std::u32string ToUTF32String(const String& str)
	{
		std::u32string result;

		for (uint32 i = 0; i < str.size(); ++i)
		{
			const char* inChar = &str[i];
			unsigned int u32Char = (unsigned int)*inChar;

			if (u32Char < 0x80)
			{
				result.push_back(u32Char);
				continue;
			}

			static const char lengths[32] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0 };
			static const int masks[] = { 0x00, 0x7f, 0x1f, 0x0f, 0x07 };
			static const uint32_t mins[] = { 0x400000, 0, 0x80, 0x800, 0x10000 };
			static const int shiftc[] = { 0, 18, 12, 6, 0 };
			static const int shifte[] = { 0, 6, 4, 2, 0 };
			int len = lengths[*(const unsigned char*)inChar >> 3];
			int wanted = len + (len ? 0 : 1);

			// Copy at most 'len' bytes, stop copying at 0 or past in_text_end. Branch predictor does a good job here,
			// so it is fast even with excessive branching.
			unsigned char s[4];
			s[0] = i + 0 < str.size() ? str[i + 0] : 0;
			s[1] = i + 1 < str.size() ? str[i + 1] : 0;
			s[2] = i + 2 < str.size() ? str[i + 2] : 0;
			s[3] = i + 3 < str.size() ? str[i + 3] : 0;

			// Assume a four-byte character and load four bytes. Unused bits are shifted out.
			u32Char = (uint32_t)(s[0] & masks[len]) << 18;
			u32Char |= (uint32_t)(s[1] & 0x3f) << 12;
			u32Char |= (uint32_t)(s[2] & 0x3f) << 6;
			u32Char |= (uint32_t)(s[3] & 0x3f) << 0;
			u32Char >>= shiftc[len];

			// Accumulate the various error conditions.
			int e = 0;
			e = (u32Char < mins[len]) << 6; // non-canonical encoding
			e |= ((u32Char >> 11) == 0x1b) << 7;  // surrogate half?
			e |= (u32Char > UNICODE_CODEPOINT_MAX) << 8;  // out of range?
			e |= (s[1] & 0xc0) >> 2;
			e |= (s[2] & 0xc0) >> 4;
			e |= (s[3]) >> 6;
			e ^= 0x2a; // top two bits of each tail byte correct?
			e >>= shifte[len];

			if (e)
			{
				// No bytes are consumed when *in_text == 0 || in_text == in_text_end.
				// One byte is consumed in case of invalid first byte of in_text.
				// All available bytes (at most `len` bytes) are consumed on incomplete/invalid second to last bytes.
				// Invalid or incomplete input may consume less bytes than wanted, therefore every byte has to be inspected in s.
				wanted = Math::Min(wanted, !!s[0] + !!s[1] + !!s[2] + !!s[3]);
				u32Char = UNICODE_CODEPOINT_INVALID;
			}

			i += wanted - 1;
			result.push_back(u32Char);
		}

		return result;
	}
}
