#pragma once

#include "Athena/Math/Types/Matrix.h"
#include "Common.h"


namespace Athena::Math
{
	template <typename T, SIZE_T Column, SIZE_T Row>
	constexpr bool All(const Matrix<T, Column, Row>& mat)
	{
		for (SIZE_T i = 0; i < Column; ++i)
		{
			for (SIZE_T j = 0; j < Row; ++j)
			{
				if (static_cast<bool>(mat[i][j]) == false)
					return false;
			}
		}

		return true;
	}

	template <typename T, SIZE_T Column, SIZE_T Row>
	constexpr bool Any(const Matrix<T, Column, Row>& mat)
	{
		for (SIZE_T i = 0; i < Column; ++i)
		{
			for (SIZE_T j = 0; j < Row; ++j)
			{
				if (static_cast<bool>(mat[i][j]) == true)
					return true;
			}
		}

		return false;
	}

	template <typename T, SIZE_T Column, SIZE_T Row>
	constexpr void Swap(const Matrix<T, Column, Row>& left, const Matrix<T, Column, Row>& right)
	{
		for (SIZE_T i = 0; i < Column; ++i)
			Swap(left[i], right[i]);
	}
}
