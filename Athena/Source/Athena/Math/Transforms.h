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
	constexpr Matrix<T, 4, 4> ScaleMatrix(const Vector<T, 3>& vec3)
	{
		Matrix<T, 4, 4> out = Matrix<T, 4, 4>::Identity();
		out[0][0] = vec3.x;
		out[1][1] = vec3.y;
		out[2][2] = vec3.z;
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
	inline Matrix<T, 4, 4> RotateMatrix(const Quaternion<T>& quat)
	{
		return quat.AsMatrix();
	}

	template <typename T>
	inline Matrix<T, 4, 4> RotateMatrix(const Vector<T, 3>& eulerAngles)
	{
		T c1 = Math::Cos(-eulerAngles.x);
		T c2 = Math::Cos(-eulerAngles.y);
		T c3 = Math::Cos(-eulerAngles.z);
		T s1 = Math::Sin(-eulerAngles.x);
		T s2 = Math::Sin(-eulerAngles.y);
		T s3 = Math::Sin(-eulerAngles.z);

		Matrix<T, 4, 4> result;

		result[0][0] = c2 * c3;
		result[0][1] = -c1 * s3 + s1 * s2 * c3;
		result[0][2] = s1 * s3 + c1 * s2 * c3;
		result[0][3] = static_cast<T>(0);
		result[1][0] = c2 * s3;
		result[1][1] = c1 * c3 + s1 * s2 * s3;
		result[1][2] = -s1 * c3 + c1 * s2 * s3;
		result[1][3] = static_cast<T>(0);
		result[2][0] = -s2;
		result[2][1] = s1 * c2;
		result[2][2] = c1 * c2;
		result[2][3] = static_cast<T>(0);
		result[3][0] = static_cast<T>(0);
		result[3][1] = static_cast<T>(0);
		result[3][2] = static_cast<T>(0);
		result[3][3] = static_cast<T>(1);
		return result;
	}

	template <typename T>
	inline Matrix<T, 4, 4> ConstructTransform(const Vector<T, 3>& translation, const Vector<T, 3>& scale, const Quaternion<T>& rotation)
	{
		return RotateMatrix(rotation).Translate(translation).Scale(scale);
	}

	template <typename T>
	inline void DecomposeTransform(const Matrix<T, 4, 4>& transform, Vector<T, 3>& translation, Vector<T, 3>& rotation, Vector<T, 3>& scale)
	{
		translation = transform[3];

		Vector<T, 3> rows[3];
		for (uint32 i = 0; i < 3; ++i)
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
	inline Matrix<T, 4, 4> LookAt(const Vector<T, 3>& eye, const Vector<T, 3>& center, const Vector<T, 3>& up)
	{
		const Vector<T, 3> f((center - eye).GetNormalized());
		const Vector<T, 3> s(Math::Cross(f, up).GetNormalized());
		const Vector<T, 3> u(Math::Cross(s, f));
		
		Matrix<T, 4, 4> result = Matrix<T, 4, 4>::Identity();
		result[0][0] = s.x;
		result[1][0] = s.y;
		result[2][0] = s.z;
		result[0][1] = u.x;
		result[1][1] = u.y;
		result[2][1] = u.z;
		result[0][2] = -f.x;
		result[1][2] = -f.y;
		result[2][2] = -f.z;
		result[3][0] = -Math::Dot(s, eye);
		result[3][1] = -Math::Dot(u, eye);
		result[3][2] = Math::Dot(f, eye);
		return result;
	}
}
