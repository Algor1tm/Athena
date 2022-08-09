#pragma once

#include <cmath>


namespace Athena
{
	template <typename X, typename Y, typename Z>
	constexpr X Clamp(X value, Y min, Z max)
	{
		if (value < static_cast<X>(min))
			return static_cast<X>(min);
		else if (value > static_cast<X>(max))
			return static_cast<X>(max);
		return value;
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
	constexpr T Abs(T value)
	{
		return value >= 0 ? value : -value;
	}

	template <typename T>
	constexpr T Sign(T value)
	{
		if (value > 0)
			return static_cast<T>(1);
		else if (value < 0)
			return static_cast<T>(-1);
		return static_cast<T>(0);
	}



	template <typename T>
	constexpr T Round(T value)
	{
		return std::round(value);
	}

	template <typename T>
	constexpr T Floor(T value)
	{
		return std::floor(value);
	}

	template <typename T>
	constexpr T Ceil(T value)
	{
		return std::ceil(value);
	}

	template <typename T>
	constexpr T Trunc(T value)
	{
		return std::trunc(value);
	}

	template <typename T>
	constexpr T Fract(T value)
	{
		return Abs(value - std::trunc(value));
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
			return Max(val1, std::forward<Args>(others)...);

		return Max(val2, std::forward<Args>(others)...);
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
			return Min(val1, std::forward<Args>(others)...);

		return Min(val2, std::forward<Args>(others)...);
	}
}
