#pragma once


#include "TypesImpl/Vector.h"
#include "TypesImpl/Vector2.h"
#include "TypesImpl/Vector3.h"
#include "TypesImpl/Vector4.h"

#include "VectorCommon.h"
#include "VectorTrigonometric.h"
#include "VectorExponential.h"


namespace Athena
{
	template <typename T, SIZE_T Size>
	using Vector = Athena::Math::Vector<T, Size>;

	using Vector2  = Vector<float, 2>;
	using Vector2f = Vector<float, 2>;
	using Vector2u = Vector<uint32, 2>;
	using Vector2i = Vector<int32, 2>;
	using Vector2d = Vector<double, 2>;

	using Vector3  = Vector<float, 3>;
	using Vector3f = Vector<float, 3>;
	using Vector3u = Vector<uint32, 3>;
	using Vector3i = Vector<int32, 3>;
	using Vector3d = Vector<double, 3>;

	using Vector4  = Vector<float, 4>;
	using Vector4f = Vector<float, 4>;
	using Vector4u = Vector<uint32, 4>;
	using Vector4i = Vector<int32, 4>;
	using Vector4d = Vector<double, 4>;
}
