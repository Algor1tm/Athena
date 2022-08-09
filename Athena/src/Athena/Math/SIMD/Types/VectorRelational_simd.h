#pragma once

#include "Vector4_float.h"


namespace Athena
{
	inline float Dot(const Vector<float, 4>& left, const Vector<float, 4>& right)
	{
		__m128 mul = _mm_mul_ps(left._xmm, right._xmm);
		__m128 swp0 = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1));
		__m128 halfsum = _mm_add_ps(mul, swp0);
		__m128 swp1 = _mm_shuffle_ps(halfsum, halfsum, _MM_SHUFFLE(0, 1, 2, 3));
		__m128 sum = _mm_add_ps(halfsum, swp1);

		return _mm_cvtss_f32(sum);
	}
}
