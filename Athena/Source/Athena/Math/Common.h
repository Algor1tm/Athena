#pragma once

#include <cmath>


namespace Athena::Math
{
	template <typename X, typename Y, typename Z>
	constexpr X Clamp(X scalar, Y min, Z max)
	{
		if (scalar < static_cast<X>(min))
			return static_cast<X>(min);
		else if (scalar > static_cast<X>(max))
			return static_cast<X>(max);

		return scalar;
	}

	template <typename T>
	constexpr T Lerp(T a, T b, T t)
	{
		return a + (b - a) * t;
	}

	template <typename T>
	constexpr T InverseLerp(T lerp, T a, T b)
	{
		return (lerp - a) / (b - a);
	}



	template <typename T>
	constexpr T Abs(T scalar)
	{
		return scalar >= 0 ? scalar : -scalar;
	}

	template <typename T>
	constexpr T Sign(T scalar)
	{
		if (scalar > 0)
			return static_cast<T>(1);
		else if (scalar < 0)
			return static_cast<T>(-1);

		return static_cast<T>(0);
	}

	template <typename T>
	constexpr bool All(T left, T right)
	{
		return (left != static_cast<T>(0) && right != static_cast<T>(0)) ? true : false;
	}

	template <typename T, typename... Args>
	constexpr bool All(T left, T right, Args... others)
	{
		if (left != 0)
			return All(right, others...);
		else
			return false;
	}

	template <typename T>
	constexpr bool Any(T left, T right)
	{
		return (left != static_cast<T>(0) || right != static_cast<T>(0)) ? true : false;
	}

	template <typename T, typename... Args>
	constexpr bool Any(T left, T right, Args... others)
	{
		if (left == 0)
			return Any(right, others...);
		else
			return true;
	}



	template <typename T>
	constexpr T Round(T scalar)
	{
		return std::round(scalar);
	}

	template <typename T>
	constexpr T Floor(T scalar)
	{
		return std::floor(scalar);
	}

	template <typename T>
	constexpr T Ceil(T scalar)
	{
		return std::ceil(scalar);
	}

	template <typename T>
	constexpr T Trunc(T scalar)
	{
		return std::trunc(scalar);
	}

	template <typename T>
	constexpr T Fract(T scalar)
	{
		return scalar - Trunc(scalar);
	}

	template <typename T>
	constexpr T Mod(T left, T right)
	{
		return left - right * Math::Floor(left / right);
	}

	template <typename T>
	constexpr T FMod(T left, T right)
	{
		return left - right * Math::Trunc(left / right);
	}

	template <typename T>
	constexpr T Modf(T scalar, T& intpart)
	{
		return std::modf(scalar, &intpart);
	}



	template <typename T>
	constexpr T Max(T left, T right)
	{
		return left >= right ? left : right;
	}

	template <typename T, typename... Args>
	constexpr T Max(T val1, T val2, Args... others)
	{
		if (val1 >= val2)
			return Math::Max(val1, others...);

		return Math::Max(val2, others...);
	}

	template <typename T>
	constexpr T Min(T left, T right)
	{
		return left <= right ? left : right;
	}

	template <typename T, typename... Args>
	constexpr T Min(T val1, T val2, Args... others)
	{
		if (val1 <= val2)
			return Math::Min(val1, others...);

		return Math::Min(val2, others...);
	}

	template <typename T>
	constexpr void Swap(T& left, T& right)
	{
		std::swap(left, right);
	}
}
