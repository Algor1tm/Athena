#pragma once


#include "Types/Matrix.h"
#include "Types/MatrixRelational.h"
#include "Types/MatrixTransforms.h"

#ifndef NO_SEE2
#include "SIMD/Types/MatrixRelational_simd.h"
#endif

namespace Athena
{
	typedef Matrix<float, 2, 2> Matrix2;
	typedef Matrix<float, 3, 3> Matrix3;
	typedef Matrix<float, 4, 4> Matrix4;

	typedef Matrix<int, 2, 2> Matrix2i;
	typedef Matrix<int, 3, 3> Matrix3i;
	typedef Matrix<int, 4, 4> Matrix4i;

	typedef Matrix<float, 2, 3> Matrix2x3;
	typedef Matrix<float, 3, 2> Matrix3x2;
	typedef Matrix<float, 2, 4> Matrix2x4;
	typedef Matrix<float, 4, 2> Matrix4x2;
	typedef Matrix<float, 3, 4> Matrix3x4;
	typedef Matrix<float, 4, 3> Matrix4x3;

	typedef Matrix<int, 2, 3> Matrix2x3i;
	typedef Matrix<int, 3, 2> Matrix3x2i;
	typedef Matrix<int, 2, 4> Matrix2x4i;
	typedef Matrix<int, 4, 2> Matrix4x2i;
	typedef Matrix<int, 3, 4> Matrix3x4i;
	typedef Matrix<int, 4, 3> Matrix4x3i;
}

