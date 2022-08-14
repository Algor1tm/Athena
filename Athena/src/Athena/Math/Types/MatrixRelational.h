#pragma once

#include "Matrix.h"
#include "VectorRelational.h"

#include <sstream>


namespace Athena
{
	template <typename T, SIZE_T Column, SIZE_T Row>
	constexpr Matrix<T, Column, Row> operator+(float scalar, const Matrix<T, Column, Row>& mat)
	{
		return mat + scalar;
	}

	template <typename T, SIZE_T Column, SIZE_T Row>
	constexpr Matrix<T, Column, Row> operator*(float scalar, const Matrix<T, Column, Row>& mat)
	{
		return mat * scalar;
	}

	template <typename T, SIZE_T vecRow, SIZE_T Column, SIZE_T Row>
	constexpr Vector<T, Row> operator*(
		const Vector<T, vecRow>& vec, const Matrix<T, Column, Row>& mat)
	{
		static_assert(vecRow == Column, "Invalid Vector Matrix multiplication");

		Vector<T, Row> out(static_cast<T>(0));
		for (SIZE_T i = 0; i < Column; i++)
		{
			for (SIZE_T j = 0; j < Row; ++j)
				out[j] += vec[i] * mat[i][j];
		}
		return out;
	}

	template <typename T, SIZE_T Column, SIZE_T Row>
	constexpr Matrix<T, Row, Column> Transpose(const Matrix<T, Column, Row>& mat)
	{
		Matrix<T, Row, Column> out;
		for (SIZE_T i = 0; i < Column; ++i)
		{
			for (SIZE_T j = 0; j < Row; ++j)
				out[j][i] = mat[i][j];
		}
		return out;
	}
	
	template <typename T>
	constexpr Matrix<T, 4, 4> AffineInverse(const Matrix<T, 4, 4>& mat)
	{
		Matrix<T, 4, 4> out;

		out[0][0] = mat[0][0]; out[0][1] = mat[1][0]; out[0][2] = mat[2][0]; out[0][3] = 0;
		out[1][0] = mat[0][1]; out[1][1] = mat[1][1]; out[1][2] = mat[2][1]; out[1][3] = 0;
		out[2][0] = mat[0][2]; out[2][1] = mat[1][2]; out[2][2] = mat[2][2]; out[2][3] = 0;

		out[3][0] = -(mat[3][0] * out[0][0] + mat[3][1] * out[1][0] + mat[3][2] * out[2][0]);
		out[3][1] = -(mat[3][0] * out[0][1] + mat[3][1] * out[1][1] + mat[3][2] * out[2][1]);
		out[3][2] = -(mat[3][0] * out[0][2] + mat[3][1] * out[1][2] + mat[3][2] * out[2][2]);
		out[3][3] = 1;

		return out;
	}


	template <typename T, SIZE_T Column, SIZE_T Row>
	inline String ToString(const Matrix<T, Column, Row>& mat)
	{
		std::stringstream stream;
		stream << "Matrix(";
		for (SIZE_T i = 0; i < Column; ++i)
		{
			for (SIZE_T j = 0; j < Row; ++j)
			{
				stream << mat[i][j] << ", ";
			}
			if (i == Column - 1)
				stream << "Column = " << Column << ", Row = " << Row << ")";
			else
				stream << "\n       ";
		}
		return stream.str();
	}
}

#ifdef ATN_SIMD
#include "Athena/Math/SIMD/Types/MatrixRelational_simd.h"
#endif
