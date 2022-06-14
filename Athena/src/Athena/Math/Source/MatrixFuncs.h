#pragma once

#include "Matrix.h"



namespace Athena
{
	template <typename Ty, size_t Column, size_t Row>
	constexpr Matrix<Ty, Column, Row> operator+(float scalar, const Matrix<Ty, Column, Row>& mat)
	{
		return mat + scalar;
	}

	template <typename Ty, size_t Column, size_t Row>
	constexpr Matrix<Ty, Column, Row> operator*(float scalar, const Matrix<Ty, Column, Row>& mat)
	{
		return mat * scalar;
	}

	template <typename Ty, size_t vecRow, size_t Column, size_t Row>
	constexpr Vector<Ty, Row> operator*(
		const Vector<Ty, vecRow>& vec, const Matrix<Ty, Column, Row>& mat)
	{
		static_assert(vecRow == Column, "Invalid Vector Matrix multiplication");

		Vector<Ty, Row> out(static_cast<Ty>(0));
		for (size_t i = 0; i < Row; i++)
		{
			for (size_t j = 0; j < Column; ++j)
			{
				out[i] += vec[j] * mat[j][i];
			}
		}
		return out;
	}

	template <typename Ty, size_t Column, size_t Row>
	constexpr Matrix<Ty, Row, Column> Transpose(const Matrix<Ty, Column, Row>& mat)
	{
		Matrix<Ty, Row, Column> out;
		for (size_t i = 0; i < Column; ++i)
		{
			for (size_t j = 0; j < Row; ++j)
				out[j][i] = mat[i][j];
		}
		return out;
	}

	inline Matrix<float, 4, 4> QuickInverse(const Matrix<float, 4, 4>& mat)
	{
		Matrix<float, 4, 4> out;

		out[0][0] = mat[0][0]; out[0][1] = mat[1][0]; out[0][2] = mat[2][0]; out[0][3] = 0;
		out[1][0] = mat[0][1]; out[1][1] = mat[1][1]; out[1][2] = mat[2][1]; out[1][3] = 0;
		out[2][0] = mat[0][2]; out[2][1] = mat[1][2]; out[2][2] = mat[2][2]; out[2][3] = 0;

		out[3][0] = -(mat[3][0] * out[0][0] + mat[3][1] * out[1][0] + mat[3][2] * out[2][0]);
		out[3][1] = -(mat[3][0] * out[0][1] + mat[3][1] * out[1][1] + mat[3][2] * out[2][1]);
		out[3][2] = -(mat[3][0] * out[0][2] + mat[3][1] * out[1][2] + mat[3][2] * out[2][2]);
		out[3][3] = 1;

		return out;
	}

	template <typename Ty, size_t Column, size_t Row>
	constexpr std::string ToString(const Matrix<Ty, Column, Row>& mat)
	{
		std::string out = "Matrix(";
		for (size_t i = 0; i < Column; ++i)
		{
			for (size_t j = 0; j < Row; ++j)
			{
				out += std::to_string(mat[i][j]) + ", ";
			}
			if (i == Column - 1)
				out += "height = " + std::to_string(Column) + ", width = " + std::to_string(Row) + ")";
			else
				out += "\n                       ";
		}
		return out;
	}
}
