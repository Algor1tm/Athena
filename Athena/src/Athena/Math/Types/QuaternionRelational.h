#pragma once

#include "Quaternion.h"


namespace Athena
{
	template <typename T>
	constexpr Quaternion<T> operator+(T scalar, const Quaternion<T>& quat)
	{
		return quat += scalar;
	}

	template <typename T>
	constexpr Quaternion<T> operator*(T scalar, const Quaternion<T>& quat)
	{
		return quat *= scalar;
	}

	template <typename T>
	constexpr Quaternion<T> operator*(const Vector<T, 3> vec, const Quaternion<T>& quat)
	{
		return quat.GetInversed() * vec;
	}

	template <typename T>
	constexpr T Dot(const Quaternion<T>& left, const Quaternion<T>& right)
	{
		return left.w * right.w + left.x * right.x + left.y * right.y + left.z * right.z;
	}

	template <typename T>
	inline String ToString(const Quaternion<T>& quat)
	{
		std::stringstream stream;
		stream << "Quaternion(" << quat.w << ", " << quat.x << ", " << quat.y << ", " << quat.z << ")";
		
		return stream.str();
	}
}
