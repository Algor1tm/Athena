#pragma once

#include "Athena/Math/TypesImpl/Matrix.h"
#include "Common.h"


namespace Athena::Math
{
	template <typename T, uint32 Column, uint32 Row>
	constexpr bool All(const Matrix<T, Column, Row>& mat)
	{
		for (uint32 i = 0; i < Column; ++i)
		{
			for (uint32 j = 0; j < Row; ++j)
			{
				if (static_cast<bool>(mat[i][j]) == false)
					return false;
			}
		}

		return true;
	}

	template <typename T, uint32 Column, uint32 Row>
	constexpr bool Any(const Matrix<T, Column, Row>& mat)
	{
		for (uint32 i = 0; i < Column; ++i)
		{
			for (uint32 j = 0; j < Row; ++j)
			{
				if (static_cast<bool>(mat[i][j]) == true)
					return true;
			}
		}

		return false;
	}

	template <typename T, uint32 Column, uint32 Row, typename ConditionFunc>
	constexpr bool All(const Matrix<T, Column, Row>& mat, ConditionFunc cond)
	{
		for (uint32 i = 0; i < Column; ++i)
		{
			for (uint32 j = 0; j < Row; ++j)
			{
				if (cond(mat[i][j]) == false)
					return false;
			}
		}

		return true;
	}

	template <typename T, uint32 Column, uint32 Row, typename ConditionFunc>
	constexpr bool Any(const Matrix<T, Column, Row>& mat, ConditionFunc cond)
	{
		for (uint32 i = 0; i < Column; ++i)
		{
			for (uint32 j = 0; j < Row; ++j)
			{
				if (cond(mat[i][j]) == true)
					return true;
			}
		}

		return false;
	}

	template <typename T, uint32 Column, uint32 Row>
	constexpr void Swap(const Matrix<T, Column, Row>& left, const Matrix<T, Column, Row>& right)
	{
		for (uint32 i = 0; i < Column; ++i)
			Math::Swap(left[i], right[i]);
	}
}
