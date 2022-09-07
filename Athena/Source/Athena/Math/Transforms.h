#pragma once

#include "Athena/Math/TypesImpl/Matrix.h"
#include "Athena/Math/TypesImpl/Matrix4.h"
#include "Athena/Math/TypesImpl/Vector.h"
#include "Athena/Math/TypesImpl/Vector4.h"
#include "Athena/Math/TypesImpl/Quaternion.h"

#include "Common.h"
#include "MatrixCommon.h"
#include "Limits.h"


namespace Athena::Math
{
	template <typename T>
	constexpr Matrix<T, 4, 4> TranslateMatrix(const Vector<T, 3>& vec3)
	{
		Matrix<T, 4, 4> out = Matrix<T, 4, 4>::Identity();
		out[3][0] = vec3.x;
		out[3][1] = vec3.y;
		out[3][2] = vec3.z;
		return out;
	}

	template <typename T>
	inline Matrix<T, 4, 4> RotateMatrix(float radians, const Vector<T, 3>& axis)
	{
		T c = Math::Cos(radians);
		T s = Math::Sin(radians);

		Vector<T, 3> temp((T(1) - c) * axis);

		Matrix<T, 4, 4> rotateMat;
		rotateMat[0][0] = c + temp[0] * axis[0];
		rotateMat[0][1] = temp[0] * axis[1] + s * axis[2];
		rotateMat[0][2] = temp[0] * axis[2] - s * axis[1];

		rotateMat[1][0] = temp[1] * axis[0] - s * axis[2];
		rotateMat[1][1] = c + temp[1] * axis[1];
		rotateMat[1][2] = temp[1] * axis[2] + s * axis[0];

		rotateMat[2][0] = temp[2] * axis[0] + s * axis[1];
		rotateMat[2][1] = temp[2] * axis[1] - s * axis[0];
		rotateMat[2][2] = c + temp[2] * axis[2];
		
		const Matrix<T, 4, 4> m = Matrix<T, 4, 4>::Identity();
		Matrix<T, 4, 4> Result;
		Result[0] = m[0] * rotateMat[0][0] + m[1] * rotateMat[0][1] + m[2] * rotateMat[0][2];
		Result[1] = m[0] * rotateMat[1][0] + m[1] * rotateMat[1][1] + m[2] * rotateMat[1][2];
		Result[2] = m[0] * rotateMat[2][0] + m[1] * rotateMat[2][1] + m[2] * rotateMat[2][2];
		Result[3] = m[3];

		return Result;
	}

	template <typename T>
	constexpr Matrix<T, 4, 4> ScaleMatrix(const Vector<T, 3>& vec3)
	{
		Matrix<T, 4, 4> out = Matrix<T, 4, 4>::Identity();
		out[0][0] = vec3.x;
		out[1][1] = vec3.y;
		out[2][2] = vec3.z;
		return out;
	}

	template <typename T>
	inline Quaternion<T> RotateQuat(T radians, const Vector<T, 3>& axis)
	{
		Vector<T, 3> tmp = axis;

		T len = tmp.Length();
		if (Math::Abs(len - static_cast<T>(1.f)) > static_cast<T>(0.001f))
		{
			T oneOverLen = static_cast<T>(1) / len;
			tmp *= oneOverLen;
		}

		T sin = Math::Sin(radians * static_cast<T>(0.5));

		return Quaternion<T>(Math::Cos(radians * static_cast<T>(0.5)), tmp.x * sin, tmp.y * sin, tmp.z * sin);
	}

	template <typename T>
	inline void DecomposeTransform(const Matrix<T, 4, 4>& transform, Vector<T, 3>& translation, Vector<T, 3>& rotation, Vector<T, 3>& scale)
	{
		translation = transform[3];

		Vector<T, 3> rows[3];
		for (SIZE_T i = 0; i < 3; ++i)
			rows[i] = transform[i];

		scale.x = rows[0].Length();
		scale.y = rows[1].Length();
		scale.z = rows[2].Length();

		rows[0].Normalize(); 
		rows[1].Normalize();
		rows[2].Normalize();

		rotation.y = Math::Asin(-rows[0][2]);
		if (Math::Cos(rotation.y) != 0)
		{
			rotation.x = Math::Atan2(rows[1][2], rows[2][2]);
			rotation.z = Math::Atan2(rows[0][1], rows[0][0]);
		}
		else
		{
			rotation.x = Math::Atan2(-rows[2][0], rows[1][1]);
			rotation.z = 0;
		}
	}

	template <typename T>
	inline Matrix<T, 4, 4> EulerAngles(T x, T y, T z)
	{
		T c1 = Math::Cos(-x);
		T c2 = Math::Cos(-y);
		T c3 = Math::Cos(-z);
		T s1 = Math::Sin(-x);
		T s2 = Math::Sin(-y);
		T s3 = Math::Sin(-z);

		Matrix<T, 4, 4> Result;

		Result[0][0] = c2 * c3;
		Result[0][1] = -c1 * s3 + s1 * s2 * c3;
		Result[0][2] = s1 * s3 + c1 * s2 * c3;
		Result[0][3] = static_cast<T>(0);
		Result[1][0] = c2 * s3;
		Result[1][1] = c1 * c3 + s1 * s2 * s3;
		Result[1][2] = -s1 * c3 + c1 * s2 * s3;
		Result[1][3] = static_cast<T>(0);
		Result[2][0] = -s2;
		Result[2][1] = s1 * c2;
		Result[2][2] = c1 * c2;
		Result[2][3] = static_cast<T>(0);
		Result[3][0] = static_cast<T>(0);
		Result[3][1] = static_cast<T>(0);
		Result[3][2] = static_cast<T>(0);
		Result[3][3] = static_cast<T>(1);
		return Result;
	}
}
