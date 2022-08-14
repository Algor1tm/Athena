#pragma once

#include "Athena/Math/Types/Quaternion.h"


namespace Athena
{
	template <typename T>
	inline Quaternion<T> RotateQuat(T radians, const Vector<T, 3>& axis)
	{
		Vector<T, 3> tmp = axis;

		T len = tmp.Length();
		if (Abs(len - static_cast<T>(1.f)) > static_cast<T>(0.001f))
		{
			T oneOverLen = static_cast<T>(1) / len;
			tmp *= oneOverLen;
		}

		T sin = Sin(radians * static_cast<T>(0.5));

		return Quaternion<T>(Cos(radians * static_cast<T>(0.5)), tmp.x * sin, tmp.y * sin, tmp.z * sin);
	}
}
