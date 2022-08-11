#pragma once

#include "Athena/Math/SIMD/Platform.h"


#ifdef ATN_SSE

#include "Athena/Math/SIMD/Types/Vector4_float.h"


namespace Athena
{
	template<typename Y, typename Z>
	inline Vector<float, 4> Clamp(const Vector<float, 4>& vec, Y min, Z max)
	{
		__m128 min0 = _mm_min_ps(vec._xmm, _mm_set_ps1(static_cast<float>(max)));
		__m128 max0 = _mm_max_ps(min0, _mm_set_ps1(static_cast<float>(min)));

		return Vector<float, 4>(max0);
	}

	inline Vector<float, 4> Clamp(const Vector<float, 4>& vec, const Vector<float, 4>& min, const Vector<float, 4>& max)
	{
		__m128 min0 = _mm_min_ps(vec._xmm, max._xmm);
		__m128 max0 = _mm_max_ps(min0, min._xmm);

		return Vector<float, 4>(max0);
	}

	inline Vector<float, 4> Lerp(const Vector<float, 4>& a, const Vector<float, 4>& b, float t)
	{
		Vector<float, 4> out;
		__m128 sub = _mm_sub_ps(b._xmm, a._xmm);
		sub = _mm_mul_ps(sub, _mm_set_ps1(t));
		out._xmm = _mm_add_ps(a._xmm, sub);

		return out;
	}



	inline Vector<float, 4> Abs(const Vector<float, 4>& vec)
	{
		Vector<float, 4> out;
		out._xmm = _mm_and_ps(vec._xmm, _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)));

		return out;
	}

	inline Vector<float, 4> Sign(const Vector<float, 4>& vec)
	{
		__m128 zro = _mm_setzero_ps();
		__m128 cmp0 = _mm_cmplt_ps(vec._xmm, zro);
		__m128 cmp1 = _mm_cmpgt_ps(vec._xmm, zro);
		__m128 and0 = _mm_and_ps(cmp0, _mm_set1_ps(-1.0f));
		__m128 and1 = _mm_and_ps(cmp1, _mm_set1_ps(1.0f));
		__m128 or = _mm_or_ps(and0, and1);

		return Vector<float, 4>(or);
	}



	inline Vector<float, 4> Round(const Vector<float, 4>& vec)
	{
		__m128 out = _mm_round_ps(vec._xmm, _MM_FROUND_TO_NEAREST_INT);
		return Vector<float, 4>(out);
	}

	inline Vector<float, 4> Floor(const Vector<float, 4>& vec)
	{
		__m128 out = _mm_floor_ps(vec._xmm);
		return Vector<float, 4>(out);
	}

	inline Vector<float, 4> Ceil(const Vector<float, 4>& vec)
	{
		__m128 out = _mm_ceil_ps(vec._xmm);
		return Vector<float, 4>(out);
	}

	inline Vector<float, 4> Fract(const Vector<float, 4>& vec)
	{
		__m128 flr = _mm_floor_ps(vec._xmm);
		__m128 sub = _mm_sub_ps(vec._xmm, flr);

		return Vector<float, 4>(sub);
	}

	inline float Mod(const Vector<float, 4>& left, const Vector<float, 4>& right)
	{	
		__m128 div0 = _mm_div_ps(left._xmm, right._xmm);
		__m128 flr0 = _mm_floor_ps(div0);
		__m128 mul0 = _mm_mul_ps(right._xmm, flr0);
		__m128 sub0 = _mm_sub_ps(left._xmm, mul0);

		return _mm_cvtss_f32(sub0);
	}



	inline Vector<float, 4> Max(const Vector<float, 4>& left, const Vector<float, 4>& right)
	{
		Vector<float, 4> out;
		out._xmm = _mm_max_ps(left._xmm, right._xmm);

		return Vector<float, 4>(out);
	}

	inline Vector<float, 4> Min(const Vector<float, 4>& left, const Vector<float, 4>& right)
	{
		Vector<float, 4> out;
		out._xmm = _mm_min_ps(left._xmm, right._xmm);

		return Vector<float, 4>(out);
	}

	inline void Swap(Vector<float, 4>& left, Vector<float, 4>& right)
	{
		__m128 temp = left._xmm;
		left._xmm = right._xmm;
		right._xmm = temp;
	}
}

#endif // ATN_SSE
