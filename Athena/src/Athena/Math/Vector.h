#pragma once


#include "Types/Vector.h"
#include "Types/Vector2.h"
#include "Types/Vector3.h"
#include "Types/Vector4.h"
#include "Types/VectorRelational.h"
#include "Utils/VectorCommon.h"
#include "Utils/VectorTrigonometric.h"
#include "Utils/VectorExponential.h"

#include "SIMD/Platform.h"
#include "SIMD/Types/Vector4_float.h"
#include "SIMD/Types/VectorRelational_simd.h"
#include "SIMD/Utils/VectorCommon_simd.h"
#include "SIMD/Utils/VectorExponential_simd.h"


namespace Athena
{
	using Vector2 = Vector<float, 2>;
	using Vector2f = Vector<float, 2>;
	using Vector2u = Vector<uint32, 2>;
	using Vector2i = Vector<int32, 2>;
	using Vector2d = Vector<double, 2>;

	using Vector3 = Vector<float, 3>;
	using Vector3f = Vector<float, 3>;
	using Vector3u = Vector<uint32, 3>;
	using Vector3i = Vector<int32, 3>;
	using Vector3d = Vector<double, 3>;

	using Vector4 = Vector<float, 4>;
	using Vector4f = Vector<float, 4>;
	using Vector4u = Vector<uint32, 4>;
	using Vector4i = Vector<int32, 4>;
	using Vector4d = Vector<double, 4>;
}
