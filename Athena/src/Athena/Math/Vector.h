#pragma once


#include "Source/Vector.h"
#include "Source/Vector2.h"
#include "Source/Vector3.h"
#include "Source/Vector4.h"
#include "SIMD/Vector4_float.h"

#include "Source/VectorFuncs.h"
#include "SIMD/VectorFuncs_simd.h"


namespace Athena
{
	typedef Vector<float, 2> Vector2;
	typedef Vector<float, 2> Vector2f;
	typedef Vector<unsigned int, 2> Vector2u;
	typedef Vector<int, 2> Vector2i;
	typedef Vector<double, 2> Vector2d;

	typedef Vector<float, 3> Vector3;
	typedef Vector<float, 3> Vector3f;
	typedef Vector<unsigned int, 3> Vector3u;
	typedef Vector<int, 3> Vector3i;
	typedef Vector<double, 3> Vector3d;

	typedef Vector<float, 4> Vector4;
	typedef Vector<float, 4> Vector4f;
	typedef Vector<unsigned int, 4> Vector4u;
	typedef Vector<int, 4> Vector4i;
	typedef Vector<double, 4> Vector4d;
}
