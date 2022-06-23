#pragma once

#include "Athena/Math/Source/Matrix.h"
#include "Vector.h"


namespace Athena
{
	typedef Matrix<float, 4, 4> Matrix4;
	typedef Vector<float, 3> Vector3;

	inline Matrix4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
	{
		Matrix4 out(0.f);
		out[0][0] = 2.f / (right - left);
		out[1][1] = 2.f / (top - bottom);
		out[2][2] = 1.f / (zFar - zNear);
		out[3][0] = -(right + left) / (right - left);
		out[3][1] = -(top + bottom) / (top - bottom);
		out[3][2] = -zNear / (zFar - zNear);
		out[3][3] = 1.f;

		return out;
	}

	inline Matrix4 Translate(const Vector3& vec3)
	{
		Matrix4 out = Matrix4::Identity();
		out[3][0] = vec3.x;
		out[3][1] = vec3.y;
		out[3][2] = vec3.z;
		return out;
	}

	inline Matrix4 Rotate(float radians, const Vector3& vec3)
	{
		float sinx = sinf(radians * vec3.x);
		float siny = sinf(radians * vec3.y);
		float sinz = sinf(radians * vec3.z);

		float cosx = cosf(radians * vec3.x);
		float cosy = cosf(radians * vec3.y);
		float cosz = cosf(radians * vec3.z);

		Matrix4 out = { { cosy * cosz,					cosy * sinz,				      -siny,     0.f },
						{ sinx * siny * cosz - cosx * sinz, sinx * siny * sinz + cosx * cosz,   sinx * cosy, 0.f },
						{ cosx * siny * cosz + sinx * sinz,   cosx * siny * sinz - sinx * cosz, cosx * cosy, 0.f },
						{ 0.f,                          0.f,                          0.f,       1.f } };

		return out;
	}

	inline Matrix4 Scale(const Vector3& vec3)
	{
		Matrix4 out = Matrix4::Identity();
		out[0][0] = vec3.x;
		out[1][1] = vec3.y;
		out[2][2] = vec3.z;
		return out;
	}

	inline Matrix4 Translate(const Matrix4& mat4, const Vector3& vec3)
	{
		Matrix4 out(mat4);
		out[3][0] += vec3.x;
		out[3][1] += vec3.y;
		out[3][2] += vec3.z;
		return out;
	}

	inline Matrix4 Rotate(const Matrix4& mat4, float degrees, const Vector3& vec3)
	{
		return Rotate(degrees, vec3) * mat4;
	}

	inline Matrix4 Scale(const Matrix4& mat4, const Vector3& vec3)
	{
		Matrix4 out(mat4);
		out[0][0] *= vec3.x;
		out[1][1] *= vec3.y;
		out[2][2] *= vec3.z;
		return out;
	}
}

