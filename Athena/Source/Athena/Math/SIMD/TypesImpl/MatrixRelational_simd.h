#pragma once

#include "Athena/Math/SIMD/Platform.h"


#ifdef ATN_SSE_2

namespace Athena::Math
{
	inline Vector<float, 4> operator*(const Vector<float, 4>& vec, const Matrix<float, 4, 4>& mat)
	{
		__m128 vecX = _mm_shuffle_ps(vec._data, vec._data, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 vecY = _mm_shuffle_ps(vec._data, vec._data, _MM_SHUFFLE(1, 1, 1, 1));
		__m128 vecZ = _mm_shuffle_ps(vec._data, vec._data, _MM_SHUFFLE(2, 2, 2, 2));
		__m128 vecW = _mm_shuffle_ps(vec._data, vec._data, _MM_SHUFFLE(3, 3, 3, 3));

		__m128 result = _mm_mul_ps(vecX, mat[0]._data);
		result = _mm_add_ps(result, _mm_mul_ps(vecY, mat[1]._data));
		result = _mm_add_ps(result, _mm_mul_ps(vecZ, mat[2]._data));
		result = _mm_add_ps(result, _mm_mul_ps(vecW, mat[3]._data));

		return Vector<float, 4>(result);
	}

	inline Matrix<float, 4, 4> Transpose(const Matrix<float, 4, 4>& mat)
	{
		Matrix<float, 4, 4> out;
		__m128 tmp0 = _mm_shuffle_ps(mat[0]._data, mat[1]._data, 0x44);
		__m128 tmp2 = _mm_shuffle_ps(mat[0]._data, mat[1]._data, 0xEE);
		__m128 tmp1 = _mm_shuffle_ps(mat[2]._data, mat[3]._data, 0x44);
		__m128 tmp3 = _mm_shuffle_ps(mat[2]._data, mat[3]._data, 0xEE);

		out[0]._data = _mm_shuffle_ps(tmp0, tmp1, 0x88);
		out[1]._data = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
		out[2]._data = _mm_shuffle_ps(tmp2, tmp3, 0x88);
		out[3]._data = _mm_shuffle_ps(tmp2, tmp3, 0xDD);

		return out;
	}

	inline Matrix<float, 4, 4> AffineInverse(const Matrix<float, 4, 4>& mat)
	{
		Matrix<float, 4, 4> out;

		__m128 tmp1 = _mm_shuffle_ps(mat[0]._data, mat[1]._data, _MM_SHUFFLE(1, 0, 1, 0));
		__m128 tmp2 = _mm_shuffle_ps(mat[0]._data, mat[1]._data, _MM_SHUFFLE(3, 2, 3, 2));
		out[0]._data = _mm_shuffle_ps(tmp1, mat[2]._data, _MM_SHUFFLE(3, 0, 2, 0));
		out[1]._data = _mm_shuffle_ps(tmp1, mat[2]._data, _MM_SHUFFLE(3, 1, 3, 1));
		out[2]._data = _mm_shuffle_ps(tmp2, mat[2]._data, _MM_SHUFFLE(3, 2, 2, 0));

		__m128 tmp = _mm_shuffle_ps(mat[3]._data, mat[3]._data, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 row3 = _mm_mul_ps(out[0]._data, tmp);

		tmp = _mm_shuffle_ps(mat[3]._data, mat[3]._data, _MM_SHUFFLE(1, 1, 1, 1));
		row3 = _mm_add_ps(row3, _mm_mul_ps(out[1]._data, tmp));

		tmp = _mm_shuffle_ps(mat[3]._data, mat[3]._data, _MM_SHUFFLE(2, 2, 2, 2));
		row3 = _mm_add_ps(row3, _mm_mul_ps(out[2]._data, tmp));

		out[3]._data = _mm_sub_ps(_mm_set_ps(1, 0, 0, 0), row3);

		return out;
	}


	// https://github.com/g-truc/glm/blob/master/glm/simd/matrix.h
	inline Matrix<float, 4, 4> Inverse(const Matrix<float, 4, 4>& mat4)
	{
		Matrix<float, 4, 4> out;

		__m128 Fac0;
		{
			__m128 Swp0a = _mm_shuffle_ps(mat4[3]._data, mat4[2]._data, _MM_SHUFFLE(3, 3, 3, 3));
			__m128 Swp0b = _mm_shuffle_ps(mat4[3]._data, mat4[2]._data, _MM_SHUFFLE(2, 2, 2, 2));

			__m128 Swp00 = _mm_shuffle_ps(mat4[2]._data, mat4[1]._data, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(mat4[2]._data, mat4[1]._data, _MM_SHUFFLE(3, 3, 3, 3));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac0 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 Fac1;
		{
			__m128 Swp0a = _mm_shuffle_ps(mat4[3]._data, mat4[2]._data, _MM_SHUFFLE(3, 3, 3, 3));
			__m128 Swp0b = _mm_shuffle_ps(mat4[3]._data, mat4[2]._data, _MM_SHUFFLE(1, 1, 1, 1));

			__m128 Swp00 = _mm_shuffle_ps(mat4[2]._data, mat4[1]._data, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(mat4[2]._data, mat4[1]._data, _MM_SHUFFLE(3, 3, 3, 3));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac1 = _mm_sub_ps(Mul00, Mul01);
		}


		__m128 Fac2;
		{
			__m128 Swp0a = _mm_shuffle_ps(mat4[3]._data, mat4[2]._data, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 Swp0b = _mm_shuffle_ps(mat4[3]._data, mat4[2]._data, _MM_SHUFFLE(1, 1, 1, 1));

			__m128 Swp00 = _mm_shuffle_ps(mat4[2]._data, mat4[1]._data, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(mat4[2]._data, mat4[1]._data, _MM_SHUFFLE(2, 2, 2, 2));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac2 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 Fac3;
		{
			__m128 Swp0a = _mm_shuffle_ps(mat4[3]._data, mat4[2]._data, _MM_SHUFFLE(3, 3, 3, 3));
			__m128 Swp0b = _mm_shuffle_ps(mat4[3]._data, mat4[2]._data, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 Swp00 = _mm_shuffle_ps(mat4[2]._data, mat4[1]._data, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(mat4[2]._data, mat4[1]._data, _MM_SHUFFLE(3, 3, 3, 3));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac3 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 Fac4;
		{
			__m128 Swp0a = _mm_shuffle_ps(mat4[3]._data, mat4[2]._data, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 Swp0b = _mm_shuffle_ps(mat4[3]._data, mat4[2]._data, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 Swp00 = _mm_shuffle_ps(mat4[2]._data, mat4[1]._data, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(mat4[2]._data, mat4[1]._data, _MM_SHUFFLE(2, 2, 2, 2));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac4 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 Fac5;
		{
			__m128 Swp0a = _mm_shuffle_ps(mat4[3]._data, mat4[2]._data, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 Swp0b = _mm_shuffle_ps(mat4[3]._data, mat4[2]._data, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 Swp00 = _mm_shuffle_ps(mat4[2]._data, mat4[1]._data, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(mat4[2]._data, mat4[1]._data, _MM_SHUFFLE(1, 1, 1, 1));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac5 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 SignA = _mm_set_ps(1.0f, -1.0f, 1.0f, -1.0f);
		__m128 SignB = _mm_set_ps(-1.0f, 1.0f, -1.0f, 1.0f);

		__m128 Temp0 = _mm_shuffle_ps(mat4[1]._data, mat4[0]._data, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 Vec0 = _mm_shuffle_ps(Temp0, Temp0, _MM_SHUFFLE(2, 2, 2, 0));

		__m128 Temp1 = _mm_shuffle_ps(mat4[1]._data, mat4[0]._data, _MM_SHUFFLE(1, 1, 1, 1));
		__m128 Vec1 = _mm_shuffle_ps(Temp1, Temp1, _MM_SHUFFLE(2, 2, 2, 0));

		__m128 Temp2 = _mm_shuffle_ps(mat4[1]._data, mat4[0]._data, _MM_SHUFFLE(2, 2, 2, 2));
		__m128 Vec2 = _mm_shuffle_ps(Temp2, Temp2, _MM_SHUFFLE(2, 2, 2, 0));

		__m128 Temp3 = _mm_shuffle_ps(mat4[1]._data, mat4[0]._data, _MM_SHUFFLE(3, 3, 3, 3));
		__m128 Vec3 = _mm_shuffle_ps(Temp3, Temp3, _MM_SHUFFLE(2, 2, 2, 0));


		__m128 Mul00 = _mm_mul_ps(Vec1, Fac0);
		__m128 Mul01 = _mm_mul_ps(Vec2, Fac1);
		__m128 Mul02 = _mm_mul_ps(Vec3, Fac2);
		__m128 Sub00 = _mm_sub_ps(Mul00, Mul01);
		__m128 Add00 = _mm_add_ps(Sub00, Mul02);
		__m128 Inv0 = _mm_mul_ps(SignB, Add00);

		__m128 Mul03 = _mm_mul_ps(Vec0, Fac0);
		__m128 Mul04 = _mm_mul_ps(Vec2, Fac3);
		__m128 Mul05 = _mm_mul_ps(Vec3, Fac4);
		__m128 Sub01 = _mm_sub_ps(Mul03, Mul04);
		__m128 Add01 = _mm_add_ps(Sub01, Mul05);
		__m128 Inv1 = _mm_mul_ps(SignA, Add01);

		__m128 Mul06 = _mm_mul_ps(Vec0, Fac1);
		__m128 Mul07 = _mm_mul_ps(Vec1, Fac3);
		__m128 Mul08 = _mm_mul_ps(Vec3, Fac5);
		__m128 Sub02 = _mm_sub_ps(Mul06, Mul07);
		__m128 Add02 = _mm_add_ps(Sub02, Mul08);
		__m128 Inv2 = _mm_mul_ps(SignB, Add02);

		__m128 Mul09 = _mm_mul_ps(Vec0, Fac2);
		__m128 Mul10 = _mm_mul_ps(Vec1, Fac4);
		__m128 Mul11 = _mm_mul_ps(Vec2, Fac5);
		__m128 Sub03 = _mm_sub_ps(Mul09, Mul10);
		__m128 Add03 = _mm_add_ps(Sub03, Mul11);
		__m128 Inv3 = _mm_mul_ps(SignA, Add03);

		__m128 Row0 = _mm_shuffle_ps(Inv0, Inv1, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 Row1 = _mm_shuffle_ps(Inv2, Inv3, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 Row2 = _mm_shuffle_ps(Row0, Row1, _MM_SHUFFLE(2, 0, 2, 0));

		//__m128 Det0 = glm_vec4_dot(in[0], Row2);
		// Dot product
		__m128 mul = _mm_mul_ps(mat4[0]._data, Row2);
		__m128 swp0 = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1));
		__m128 halfsum = _mm_add_ps(mul, swp0);
		__m128 swp1 = _mm_shuffle_ps(halfsum, halfsum, _MM_SHUFFLE(0, 1, 2, 3));
		__m128 Det0 = _mm_add_ps(halfsum, swp1);

		__m128 Rcp0 = _mm_div_ps(_mm_set1_ps(1.0f), Det0);

		out[0] = _mm_mul_ps(Inv0, Rcp0);
		out[1] = _mm_mul_ps(Inv1, Rcp0);
		out[2] = _mm_mul_ps(Inv2, Rcp0);
		out[3] = _mm_mul_ps(Inv3, Rcp0);

		return out;
	}
}

#endif // ATN_SSE_2
