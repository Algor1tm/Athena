#pragma once

#include "Athena/Math/Source/Matrix.h"
#include "Vector4_float.h"


namespace Athena
{
	inline Vector<float, 4> operator*(const Vector<float, 4>& vec, const Matrix<float, 4, 4>& mat)
	{
		__m128 vecX = _mm_shuffle_ps(vec._xmm, vec._xmm, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 vecY = _mm_shuffle_ps(vec._xmm, vec._xmm, _MM_SHUFFLE(1, 1, 1, 1));
		__m128 vecZ = _mm_shuffle_ps(vec._xmm, vec._xmm, _MM_SHUFFLE(2, 2, 2, 2));
		__m128 vecW = _mm_shuffle_ps(vec._xmm, vec._xmm, _MM_SHUFFLE(3, 3, 3, 3));

		__m128 result = _mm_mul_ps(vecX, mat[0]._xmm);
		result = _mm_add_ps(result, _mm_mul_ps(vecY, mat[1]._xmm));
		result = _mm_add_ps(result, _mm_mul_ps(vecZ, mat[2]._xmm));
		result = _mm_add_ps(result, _mm_mul_ps(vecW, mat[3]._xmm));

		return Vector<float, 4>(result);
	}

	inline Matrix<float, 4, 4> operator*(const Matrix<float, 4, 4>& Left, const Matrix<float, 4, 4>& Right)
	{
		Matrix<float, 4, 4> out;
		out[0] = Left[0] * Right;
		out[1] = Left[1] * Right;
		out[2] = Left[2] * Right;
		out[3] = Left[3] * Right;
		return out;
	}

	inline Matrix<float, 4, 4> Transpose(const Matrix<float, 4, 4>& mat)
	{
		Matrix<float, 4, 4> out;
		__m128 tmp0 = _mm_shuffle_ps(mat[0]._xmm, mat[1]._xmm, 0x44);
		__m128 tmp2 = _mm_shuffle_ps(mat[0]._xmm, mat[1]._xmm, 0xEE);
		__m128 tmp1 = _mm_shuffle_ps(mat[2]._xmm, mat[3]._xmm, 0x44);
		__m128 tmp3 = _mm_shuffle_ps(mat[2]._xmm, mat[3]._xmm, 0xEE);

		out[0]._xmm = _mm_shuffle_ps(tmp0, tmp1, 0x88);
		out[1]._xmm = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
		out[2]._xmm = _mm_shuffle_ps(tmp2, tmp3, 0x88);
		out[3]._xmm = _mm_shuffle_ps(tmp2, tmp3, 0xDD);

		return out;
	}
}
