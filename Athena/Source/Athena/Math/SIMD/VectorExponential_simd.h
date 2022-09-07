#pragma once

#include "Athena/Math/SIMD/Platform.h"


#ifdef ATN_SSE_2

namespace Athena::Math
{
	inline Vector<float, 4> Sqrt(const Vector<float, 4>& vec)
	{
		Vector<float, 4> out;
		out._data = _mm_sqrt_ps(vec._data);

		return out;
	}
}

#endif // ATN_SSE
