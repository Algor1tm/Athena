#pragma once

#include "Athena/Math/Types/Matrix.h"


namespace Athena
{
	constexpr Matrix<float, 4, 4> Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
	{
		Matrix<float, 4, 4> out(0.f);
		out[0][0] = 2.f / (right - left);
		out[1][1] = 2.f / (top - bottom);
		out[2][2] = 1.f / (zFar - zNear);
		out[3][0] = -(right + left) / (right - left);
		out[3][1] = -(top + bottom) / (top - bottom);
		out[3][2] = -zNear / (zFar - zNear);
		out[3][3] = 1.f;

		return out;
	}
}
