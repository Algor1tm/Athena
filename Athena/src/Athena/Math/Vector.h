#pragma once


#include "Types/Vector.h"
#include "Types/Vector2.h"
#include "Types/Vector3.h"
#include "Types/Vector4.h"
#include "Types/VectorRelational.h"
#include "Utils/VectorCommon.h"
#include "Utils/VectorTrigonometric.h"
#include "Utils/VectorExponential.h"

#ifndef NO_SEE2
#include "SIMD/Types/Vector4_float.h"
#include "SIMD/Types/VectorRelational_simd.h"
#endif


namespace Athena
{
	using Vector2 = Vector<float, 2>;
	using Vector2f = Vector<float, 2>;
	using Vector2u = Vector<unsigned int, 2>;
	using Vector2i = Vector<int, 2>;
	using Vector2d = Vector<double, 2>;

	using Vector3 = Vector<float, 3>;
	using Vector3f = Vector<float, 3>;
	using Vector3u = Vector<unsigned int, 3>;
	using Vector3i = Vector<int, 3>;
	using Vector3d = Vector<double, 3>;

	using Vector4 = Vector<float, 4>;
	using Vector4f = Vector<float, 4>;
	using Vector4u = Vector<unsigned int, 4>;
	using Vector4i = Vector<int, 4>;
	using Vector4d = Vector<double, 4>;
}
