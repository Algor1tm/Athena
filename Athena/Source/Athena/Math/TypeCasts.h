#pragma once

#include "TypesImpl/Vector4.h"
#include "TypesImpl/Matrix4.h"
#include "TypesImpl/Quaternion.h"

#include "Trigonometric.h"


namespace Athena::Math
{
	template <typename T>
	Matrix<T, 3, 3> ToMat3(const Matrix<T, 4, 4>& mat4)
	{
		Matrix<T, 3, 3> out;
		out[0] = Vector<T, 3>(mat4[0]);
		out[1] = Vector<T, 3>(mat4[1]);
		out[2] = Vector<T, 3>(mat4[2]);

		return out;
	}

	template <typename T>
	Matrix<T, 3, 3> ToMat3(const Quaternion<T>& quat)
	{
		Matrix<float, 3, 3> out(T(1));
		T qxx(quat.x * quat.x);
		T qyy(quat.y * quat.y);
		T qzz(quat.z * quat.z);
		T qxz(quat.x * quat.z);
		T qxy(quat.x * quat.y);
		T qyz(quat.y * quat.z);
		T qwx(quat.w * quat.x);
		T qwy(quat.w * quat.y);
		T qwz(quat.w * quat.z);

		out[0][0] = T(1) - T(2) * (qyy + qzz);
		out[0][1] = T(2) * (qxy + qwz);
		out[0][2] = T(2) * (qxz - qwy);

		out[1][0] = T(2) * (qxy - qwz);
		out[1][1] = T(1) - T(2) * (qxx + qzz);
		out[1][2] = T(2) * (qyz + qwx);

		out[2][0] = T(2) * (qxz + qwy);
		out[2][1] = T(2) * (qyz - qwx);
		out[2][2] = T(1) - T(2) * (qxx + qyy);

		return out;
	}

	template <typename T>
	Matrix<T, 4, 4> ToMat4(const Matrix<T, 3, 3>& mat3)
	{
		Matrix<T, 4, 4> out;
		out[0] = Vector<T, 4>(mat3[0], 0);
		out[1] = Vector<T, 4>(mat3[1], 0);
		out[2] = Vector<T, 4>(mat3[2], 0);
		out[3] = Vector<T, 4>(0, 0, 0, 1);

		return out;
	}

	template <typename T>
	Matrix<T, 4, 4> ToMat4(const Quaternion<T>& quat)
	{
		Matrix<T, 3, 3> mat3 = ToMat3(quat);

		return ToMat4(mat3);
	}

	template <typename T>
	Quaternion<T> ToQuat(const Matrix<T, 3, 3>& mat3)
	{
		Quaternion<T> out;

		T fourXSquaredMinus1 = m[0][0] - m[1][1] - m[2][2];
		T fourYSquaredMinus1 = m[1][1] - m[0][0] - m[2][2];
		T fourZSquaredMinus1 = m[2][2] - m[0][0] - m[1][1];
		T fourWSquaredMinus1 = m[0][0] + m[1][1] + m[2][2];

		int biggestIndex = 0;
		T fourBiggestSquaredMinus1 = fourWSquaredMinus1;
		if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourXSquaredMinus1;
			biggestIndex = 1;
		}
		if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourYSquaredMinus1;
			biggestIndex = 2;
		}
		if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourZSquaredMinus1;
			biggestIndex = 3;
		}

		T biggestVal = sqrt(fourBiggestSquaredMinus1 + static_cast<T>(1)) * static_cast<T>(0.5);
		T mult = static_cast<T>(0.25) / biggestVal;

		switch (biggestIndex)
		{
		case 0:
			out.w = biggestVal;
			out.x = (m[1][2] - m[2][1]) * mult;
			out.y = (m[2][0] - m[0][2]) * mult;
			out.z = (m[0][1] - m[1][0]) * mult;
			break;
		case 1:
			out.w = (m[1][2] - m[2][1]) * mult;
			out.x = biggestVal;
			out.y = (m[0][1] + m[1][0]) * mult;
			out.z = (m[2][0] + m[0][2]) * mult;
			break;
		case 2:
			out.w = (m[2][0] - m[0][2]) * mult;
			out.x = (m[0][1] + m[1][0]) * mult;
			out.y = biggestVal;
			out.z = (m[1][2] + m[2][1]) * mult;
			break;
		case 3:
			out.w = (m[0][1] - m[1][0]) * mult;
			out.x = (m[2][0] + m[0][2]) * mult;
			out.y = (m[1][2] + m[2][1]) * mult;
			out.z = biggestVal;
			break;
		default:
			ATN_CORE_ASSERT(false, "");
		}

		return out;
	}

	template <typename T>
	Quaternion<T> ToQuat(const Matrix<T, 4, 4>& mat4)
	{
		return ToQuat(ToMat3(mat4));
	}

	template <typename T>
	Quaternion<T> ToQuat(const Vector<T, 3>& eulerAngles)
	{
		Quaternion<T> out;

		Vector<T, 3> c = Math::Cos(eulerAngles * T(0.5));
		Vector<T, 3> s = Math::Sin(eulerAngles * T(0.5));

		out.w = c.x * c.y * c.z + s.x * s.y * s.z;
		out.x = s.x * c.y * c.z - c.x * s.y * s.z;
		out.y = c.x * s.y * c.z + s.x * c.y * s.z;
		out.z = c.x * c.y * s.z - s.x * s.y * c.z;

		return out;
	}
}
