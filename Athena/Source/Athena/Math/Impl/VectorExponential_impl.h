#pragma once

#include "Athena/Math/Impl/Platform.h"


#ifdef ATN_SSE_2

#include "Athena/Math/Impl/Types/Vector4float_impl.h"


namespace Athena::Math
{
	inline Vector<float, 4> Sqrt(const Vector<float, 4>& vec)
	{
		Vector<float, 4> out;
		out._xmm = _mm_sqrt_ps(vec._xmm);

		return out;
	}
}

#endif // ATN_SSE
