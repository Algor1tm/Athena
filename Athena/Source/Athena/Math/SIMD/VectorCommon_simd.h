#pragma once

#include "Athena/Math/SIMD/Platform.h"


#ifdef ATN_SSE_2

namespace Athena::Math
{
	inline Vector<float, 4> Clamp(const Vector<float, 4>& vec, const Vector<float, 4>& min, const Vector<float, 4>& max)
	{
		Vector<float, 4> out;
		out._data = _mm_min_ps(_mm_max_ps(vec._data, min._data), max._data);

		return out;
	}

	inline Vector<float, 4> Lerp(const Vector<float, 4>& a, const Vector<float, 4>& b, float t)
	{
		Vector<float, 4> out;
		__m128 sub = _mm_sub_ps(b._data, a._data);
		sub = _mm_mul_ps(sub, _mm_set_ps1(t));
		out._data = _mm_add_ps(a._data, sub);

		return out;
	}



	inline Vector<float, 4> Abs(const Vector<float, 4>& vec)
	{
		Vector<float, 4> out;
		out._data = _mm_and_ps(vec._data, _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)));

		return out;
	}

	inline Vector<float, 4> Sign(const Vector<float, 4>& vec)
	{
		__m128 zr0 = _mm_setzero_ps();
		__m128 cmp0 = _mm_cmplt_ps(vec._data, zr0);
		__m128 cmp1 = _mm_cmpgt_ps(vec._data, zr0);
		__m128 and0 = _mm_and_ps(cmp0, _mm_set_ps1(-1.0f));
		__m128 and1 = _mm_and_ps(cmp1, _mm_set_ps1(1.0f));
		__m128 or0 = _mm_or_ps(and0, and1);

		return Vector<float, 4>(or0);
	}



	inline Vector<float, 4> Round(const Vector<float, 4>& vec)
	{
#ifdef ATN_SSE_4_1
		__m128 out = _mm_round_ps(vec._data, _MM_FROUND_TO_NEAREST_INT);
#else
		__m128 sgn0 = _mm_castsi128_ps(_mm_set1_epi32(int(0x80000000)));
		__m128 and0 = _mm_and_ps(sgn0, vec._data);
		__m128 or0 = _mm_or_ps(and0, _mm_set_ps1(8388608.0f));
		__m128 add0 = _mm_add_ps(vec._data, or0);
		__m128 out = _mm_sub_ps(add0, or0);
#endif
		return Vector<float, 4>(out);
	}

	inline Vector<float, 4> Floor(const Vector<float, 4>& vec)
	{
#ifdef ATN_SSE_4_1
		__m128 out = _mm_floor_ps(vec._data);
#else
		__m128 rnd0 = Round(vec)._data;
		__m128 cmp0 = _mm_cmplt_ps(vec._data, rnd0);
		__m128 and0 = _mm_and_ps(cmp0, _mm_set1_ps(1.0f));
		__m128 out = _mm_sub_ps(rnd0, and0);
#endif

		return Vector<float, 4>(out);
	}

	inline Vector<float, 4> Ceil(const Vector<float, 4>& vec)
	{
#ifdef ATN_SSE_4_1
		__m128 out = _mm_ceil_ps(vec._data);
#else
		__m128 rnd0 = Round(vec)._data;
		__m128 cmp0 = _mm_cmpgt_ps(vec._data, rnd0);
		__m128 and0 = _mm_and_ps(cmp0, _mm_set_ps1(1.0f));
		__m128 out = _mm_add_ps(rnd0, and0);
#endif

		return Vector<float, 4>(out);
	}

	inline Vector<float, 4> Fract(const Vector<float, 4>& vec)
	{
		__m128 flr = Floor(vec)._data;
		__m128 sub = _mm_sub_ps(vec._data, flr);

		return Vector<float, 4>(sub);
	}

	inline float Mod(const Vector<float, 4>& left, const Vector<float, 4>& right)
	{	
		__m128 div0 = _mm_div_ps(left._data, right._data);
		__m128 flr0 = Floor(Vector<float, 4>(div0))._data;
		__m128 mul0 = _mm_mul_ps(right._data, flr0);
		__m128 sub0 = _mm_sub_ps(left._data, mul0);

		return _mm_cvtss_f32(sub0);
	}



	inline Vector<float, 4> Max(const Vector<float, 4>& left, const Vector<float, 4>& right)
	{
		Vector<float, 4> out;
		out._data = _mm_max_ps(left._data, right._data);

		return Vector<float, 4>(out);
	}

	inline Vector<float, 4> Min(const Vector<float, 4>& left, const Vector<float, 4>& right)
	{
		Vector<float, 4> out;
		out._data = _mm_min_ps(left._data, right._data);

		return Vector<float, 4>(out);
	}

	inline void Swap(Vector<float, 4>& left, Vector<float, 4>& right)
	{
		__m128 temp = left._data;
		left._data = right._data;
		right._data = temp;
	}
}

#endif // ATN_SSE_2
