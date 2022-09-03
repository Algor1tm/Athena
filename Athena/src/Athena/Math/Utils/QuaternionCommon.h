#pragma once

#include "Athena/Math/Types/Quaternion.h"
#include "Athena/Math/Types/QuaternionRelational.h"
#include "Athena/Math/Utils/Common.h"


namespace Athena::Math
{
	template <typename T>
	constexpr Quaternion<T> Lerp(const Quaternion<T>& a, const Quaternion<T>& b, T t)
	{
		Quaternion<T> out;
		out.w = Lerp(a.w, b.w, t);
		out.x = Lerp(a.x, b.x, t);
		out.y = Lerp(a.y, b.y, t);
		out.z = Lerp(a.z, b.z, t);

		return out;
	}

	template <typename T>
	constexpr Quaternion<T> SLerp(const Quaternion<T>& a, const Quaternion<T>& b, T t)
	{
		T cosTheta = Dot(a, b);

		if (cosTheta > static_cast<T>(1) - static_cast<T>(0.001f))
		{
			return Lerp(a, b, t);
		}
		else
		{
			T angle = Acos(cosTheta);
			return (Sin((static_cast<T>(1) - t) * angle) * a + Sin(t * angle) * b) / Sin(angle);
		}
	}

	template <typename T>
	constexpr void Swap(Quaternion<T>& left, Quaternion<T>& right)
	{
		Swap(left.w, right.w);
		Swap(left.x, right.x);
		Swap(left.y, right.y);
		Swap(left.z, right.z);
	}
}
