#pragma once

#include "Athena/Math/TypesImpl/Quaternion.h"

#include "Common.h"
#include "Trigonometric.h"


namespace Athena::Math
{
	template <typename T>
	constexpr Quaternion<T> Lerp(const Quaternion<T>& a, const Quaternion<T>& b, T t)
	{
		Quaternion<T> out;
		out.w = Math::Lerp(a.w, b.w, t);
		out.x = Math::Lerp(a.x, b.x, t);
		out.y = Math::Lerp(a.y, b.y, t);
		out.z = Math::Lerp(a.z, b.z, t);

		return out;
	}

	template <typename T>
	constexpr Quaternion<T> SLerp(const Quaternion<T>& a, const Quaternion<T>& b, T t)
	{
		T cosTheta = Math::Dot(a, b);

		Quaternion<T> result = b;
		if (cosTheta < static_cast<T>(0))
		{
			cosTheta = -cosTheta;
			result.w = -result.w;
			result.x = -result.x;
			result.y = -result.y;
			result.z = -result.z;
		}

		T scaleA, scaleB;
		if (cosTheta > static_cast<T>(1) - static_cast<T>(0.0001))
		{
			scaleA = static_cast<T>(1) - t;
			scaleB = t;
		}
		else
		{
			T angle = Math::Acos(cosTheta);
			T sinAngle = Math::Sin(angle);
			scaleA = Math::Sin((static_cast<T>(1) - t) * angle) / sinAngle;
			scaleB = Math::Sin(t * angle) / sinAngle;
		}

		result = scaleA * a + scaleB * result;
		return result;
	}

	template <typename T>
	constexpr void Swap(Quaternion<T>& left, Quaternion<T>& right)
	{
		Math::Swap(left.w, right.w);
		Math::Swap(left.x, right.x);
		Math::Swap(left.y, right.y);
		Math::Swap(left.z, right.z);
	}
}
