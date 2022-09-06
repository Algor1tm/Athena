#pragma once

#include "Athena/Math/Impl/Platform.h"


#ifdef ATN_SSE_2

#include "Athena/Math/Types/Matrix.h"
#include "Athena/Math/Impl/Types/Vector4float_impl.h"


namespace Athena::Math
{
	inline Vector<float, 4> operator*(const Vector<float, 4>& vec, const Matrix<float, 4, 4>& mat);

	inline Matrix<float, 4, 4> operator*(const Matrix<float, 4, 4>& Left, const Matrix<float, 4, 4>& Right);

	inline Matrix<float, 4, 4> Transpose(const Matrix<float, 4, 4>& mat);

	inline Matrix<float, 4, 4> AffineInverse(const Matrix<float, 4, 4>& mat);
}

#endif // ATN_SSE
