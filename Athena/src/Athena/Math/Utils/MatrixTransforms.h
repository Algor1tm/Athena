#pragma once

#include "Athena/Math/Types/Matrix.h"
#include "Athena/Math/Types/Vector.h"
#include "Common.h"


namespace Athena
{
	template <typename T>
	constexpr Matrix<T, 4, 4> Translate(const Vector<T, 3>& vec3)
	{
		Matrix<T, 4, 4> out = Matrix<T, 4, 4>::Identity();
		out[3][0] = vec3.x;
		out[3][1] = vec3.y;
		out[3][2] = vec3.z;
		return out;
	}

	template <typename T>
	inline Matrix<T, 4, 4> Rotate(float radians, const Vector<T, 3>& axis)
	{
		T sinx = Sin(radians * axis.x);
		T siny = Sin(radians * axis.y);
		T sinz = Sin(radians * axis.z);

		T cosx = Cos(radians * axis.x);
		T cosy = Cos(radians * axis.y);
		T cosz = Cos(radians * axis.z);

		Matrix<T, 4, 4> out = 
						{ { cosy * cosz,					cosy * sinz,				      -siny,     0.f },
						{ sinx * siny * cosz - cosx * sinz, sinx * siny * sinz + cosx * cosz,   sinx * cosy, 0.f },
						{ cosx * siny * cosz + sinx * sinz,   cosx * siny * sinz - sinx * cosz, cosx * cosy, 0.f },
						{ 0.f,                          0.f,                          0.f,       1.f } };

		return out;
	}

	template <typename T>
	constexpr Matrix<T, 4, 4> Scale(const Vector<T, 3>& vec3)
	{
		Matrix<T, 4, 4> out = Matrix<T, 4, 4>::Identity();
		out[0][0] = vec3.x;
		out[1][1] = vec3.y;
		out[2][2] = vec3.z;
		return out;
	}

	template <typename T>
	constexpr Matrix<T, 4, 4> Translate(const Matrix<T, 4, 4>& mat4, const Vector<T, 3>& vec3)
	{
		Matrix<T, 4, 4> out(mat4);
		out[3][0] += vec3.x;
		out[3][1] += vec3.y;
		out[3][2] += vec3.z;
		return out;
	}
	
	template <typename T>
	inline Matrix<T, 4, 4> Rotate(const Matrix<T, 4, 4>& mat4, float degrees, const Vector<T, 3>& vec3)
	{
		return Rotate(degrees, vec3) * mat4;
	}

	template <typename T>
	constexpr Matrix<T, 4, 4> Scale(const Matrix<T, 4, 4>& mat4, const Vector<T, 3>& vec3)
	{
		Matrix<T, 4, 4> out(mat4);
		out[0][0] *= vec3.x;
		out[1][1] *= vec3.y;
		out[2][2] *= vec3.z;
		return out;
	}
}

