#pragma once

#include "Matrix.h"
#include "Vector3.h"


namespace Athena
{
	Matrix4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar);

	Matrix4 Translate(const Vector3& vec3);
	Matrix4 Rotate(float degrees, const Vector3& vec3);
	Matrix4 Scale(const Vector3& vec3);

	// For transformation matrices( last row = (0, 0, 0, 1) ).
	Matrix4 QuickInverse(const Matrix4& mat);
}

