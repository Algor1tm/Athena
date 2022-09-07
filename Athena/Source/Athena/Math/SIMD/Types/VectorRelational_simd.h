#pragma once

#include "Athena/Math/SIMD/Platform.h"


#ifdef ATN_SSE_2

namespace Athena::Math
{
	inline float Dot(const Vector<float, 4>& left, const Vector<float, 4>& right)
	{
		__m128 mul = _mm_mul_ps(left._data, right._data);
		__m128 swp0 = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1));
		__m128 halfsum = _mm_add_ps(mul, swp0);
		__m128 swp1 = _mm_shuffle_ps(halfsum, halfsum, _MM_SHUFFLE(0, 1, 2, 3));
		__m128 sum = _mm_add_ps(halfsum, swp1);

		return _mm_cvtss_f32(sum);
	}

	inline Vector<float, 3> Cross(const Vector<float, 3>& left, const Vector<float, 3>& right)
	{
		__m128 set0 = _mm_set_ps(0.0f, left.z, left.y, left.x);
		__m128 set1 = _mm_set_ps(0.0f, right.z, right.y, right.x);

		__m128 swp0 = _mm_shuffle_ps(set0, set0, _MM_SHUFFLE(3, 0, 2, 1));
		__m128 swp1 = _mm_shuffle_ps(set0, set0, _MM_SHUFFLE(3, 1, 0, 2));
		__m128 swp2 = _mm_shuffle_ps(set1, set1, _MM_SHUFFLE(3, 0, 2, 1));
		__m128 swp3 = _mm_shuffle_ps(set1, set1, _MM_SHUFFLE(3, 1, 0, 2));
		__m128 mul0 = _mm_mul_ps(swp0, swp3);
		__m128 mul1 = _mm_mul_ps(swp1, swp2);
		__m128 sub0 = _mm_sub_ps(mul0, mul1);

		Vector<float, 4> result(sub0);

		return Vector<float, 3>(result);
	}
}

#endif //ATN_SSE_2
