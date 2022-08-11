#pragma once

#include "Athena/Math/SIMD/Platform.h"


#ifdef ATN_SSE

#include "Athena/Math/SIMD/Types/Vector4_float.h"


namespace Athena
{
	inline Vector<float, 4> Sqrt(const Vector<float, 4>& vec)
	{
		Vector<float, 4> out;
		out._xmm = _mm_sqrt_ps(vec._xmm);

		return out;
	}
}

#endif // ATN_SSE
