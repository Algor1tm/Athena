#pragma once

#include "Athena/Math/Types/Matrix.h"
#include "Athena/Math/Types/Vector.h"


namespace Athena
{
	constexpr Matrix<float, 4, 4> Translate(const Vector3& vec3)
	{
		Matrix<float, 4, 4> out = Matrix<float, 4, 4>::Identity();
		out[3][0] = vec3.x;
		out[3][1] = vec3.y;
		out[3][2] = vec3.z;
		return out;
	}

	inline Matrix<float, 4, 4> Rotate(float radians, const Vector3& vec3)
	{
		float sinx = sinf(radians * vec3.x);
		float siny = sinf(radians * vec3.y);
		float sinz = sinf(radians * vec3.z);

		float cosx = cosf(radians * vec3.x);
		float cosy = cosf(radians * vec3.y);
		float cosz = cosf(radians * vec3.z);

		Matrix<float, 4, 4> out = { { cosy * cosz,					cosy * sinz,				      -siny,     0.f },
						{ sinx * siny * cosz - cosx * sinz, sinx * siny * sinz + cosx * cosz,   sinx * cosy, 0.f },
						{ cosx * siny * cosz + sinx * sinz,   cosx * siny * sinz - sinx * cosz, cosx * cosy, 0.f },
						{ 0.f,                          0.f,                          0.f,       1.f } };

		return out;
	}

	constexpr Matrix<float, 4, 4> Scale(const Vector3& vec3)
	{
		Matrix<float, 4, 4> out = Matrix<float, 4, 4>::Identity();
		out[0][0] = vec3.x;
		out[1][1] = vec3.y;
		out[2][2] = vec3.z;
		return out;
	}

	constexpr Matrix<float, 4, 4> Translate(const Matrix<float, 4, 4>& mat4, const Vector3& vec3)
	{
		Matrix<float, 4, 4> out(mat4);
		out[3][0] += vec3.x;
		out[3][1] += vec3.y;
		out[3][2] += vec3.z;
		return out;
	}

	inline Matrix<float, 4, 4> Rotate(const Matrix<float, 4, 4>& mat4, float degrees, const Vector3& vec3)
	{
		return Rotate(degrees, vec3) * mat4;
	}

	constexpr Matrix<float, 4, 4> Scale(const Matrix<float, 4, 4>& mat4, const Vector3& vec3)
	{
		Matrix<float, 4, 4> out(mat4);
		out[0][0] *= vec3.x;
		out[1][1] *= vec3.y;
		out[2][2] *= vec3.z;
		return out;
	}
}

