#pragma once


#include "Types/Matrix.h"
#include "Types/Matrix4.h"
#include "MatrixTransforms.h"
#include "MatrixProjection.h"
#include "MatrixCommon.h"


namespace Athena
{
	template <typename T, SIZE_T Column, SIZE_T Row>
	using Matrix = Athena::Math::Matrix<T, Column, Row>;

	using Matrix2 = Matrix<float, 2, 2>;
	using Matrix3 = Matrix<float, 3, 3>;
	using Matrix4 = Matrix<float, 4, 4>;

	using Matrix2f = Matrix<float, 2, 2>;
	using Matrix3f = Matrix<float, 3, 3>;
	using Matrix4f = Matrix<float, 4, 4>;

	using Matrix2i = Matrix<int32, 2, 2>;
	using Matrix3i = Matrix<int32, 3, 3>;
	using Matrix4i = Matrix<int32, 4, 4>;

	using Matrix2d = Matrix<double, 2, 2>;
	using Matrix3d = Matrix<double, 3, 3>;
	using Matrix4d = Matrix<double, 4, 4>;

	using Matrix2x3 = Matrix<float, 2, 3>;
	using Matrix3x2 = Matrix<float, 3, 2>;
	using Matrix2x4 = Matrix<float, 2, 4>;
	using Matrix4x2 = Matrix<float, 4, 2>;
	using Matrix3x4 = Matrix<float, 3, 4>;
	using Matrix4x3 = Matrix<float, 4, 3>;

	using Matrix2x3f = Matrix<float, 2, 3>;
	using Matrix3x2f = Matrix<float, 3, 2>;
	using Matrix2x4f = Matrix<float, 2, 4>;
	using Matrix4x2f = Matrix<float, 4, 2>;
	using Matrix3x4f = Matrix<float, 3, 4>;
	using Matrix4x3f = Matrix<float, 4, 3>;

	using Matrix2x3i = Matrix<int32, 2, 3>;
	using Matrix3x2i = Matrix<int32, 3, 2>;
	using Matrix2x4i = Matrix<int32, 2, 4>;
	using Matrix4x2i = Matrix<int32, 4, 2>;
	using Matrix3x4i = Matrix<int32, 3, 4>;
	using Matrix4x3i = Matrix<int32, 4, 3>;

	using Matrix2x3d = Matrix<double, 2, 3>;
	using Matrix3x2d = Matrix<double, 3, 2>;
	using Matrix2x4d = Matrix<double, 2, 4>;
	using Matrix4x2d = Matrix<double, 4, 2>;
	using Matrix3x4d = Matrix<double, 3, 4>;
	using Matrix4x3d = Matrix<double, 4, 3>;
}

