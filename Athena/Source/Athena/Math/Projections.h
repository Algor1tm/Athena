#pragma once

#include "Athena/Math/TypesImpl/Matrix.h"
#include "Athena/Math/TypesImpl/Matrix4.h"


namespace Athena::Math
{
	template <typename T>
	constexpr Matrix<T, 4, 4> Ortho(T left, T right, T bottom, T top, T zNear, T zFar)
	{
		Matrix<T, 4, 4> out(0.f);
		out[0][0] = 2.f / (right - left);
		out[1][1] = 2.f / (top - bottom);
		out[2][2] = 1.f / (zFar - zNear);
		out[3][0] = -(right + left) / (right - left);
		out[3][1] = -(top + bottom) / (top - bottom);
		out[3][2] = -zNear / (zFar - zNear);
		out[3][3] = 1.f;

		return out;
	}

	template <typename T>
	inline Matrix<T, 4, 4> Perspective(T verticalFOV, T aspectRatio, T zNear, T zFar)
	{
		Matrix<T, 4, 4> out(0.f);
		T invTan = T(1) / Math::Tan(verticalFOV / 2);
		out[0][0] = invTan / aspectRatio;
		out[1][1] = invTan;
		out[2][2] = -(zFar + zNear) / (zFar - zNear);
		out[3][2] = T(-2) * (zFar * zNear) / (zFar - zNear);
		out[2][3] = T(-1);

		return out;
	}
}
