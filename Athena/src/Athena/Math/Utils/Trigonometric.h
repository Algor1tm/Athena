#pragma once

#include <cmath>

#define M_PI 3.14159265358979323846
#define M_PIf 3.14159265358979323846f


namespace Athena
{
	template <typename T>
	constexpr T PI()
	{
		return static_cast<T>(M_PI);
	}

	template<typename T>
	constexpr T Radians(T degrees)
	{
		return degrees * PI<T>() / static_cast<T>(180);
	}

	constexpr float Radians(int degrees)
	{
		return float(degrees) * PI<float>() / 180.f;
	}

	template<typename T>
	constexpr T Degrees(T radians)
	{
		return radians * static_cast<T>(180) / PI<T>();
	}


	template<typename T>
	inline T Cos(T scalar)
	{
		return std::cos(scalar);
	}

	template<typename T>
	inline T Sin(T scalar)
	{
		return std::sin(scalar);
	}

	template<typename T>
	inline T Tan(T scalar)
	{
		return std::tan(scalar);
	}

	template<typename T>
	inline T Acos(T scalar)
	{
		return std::acos(scalar);
	}

	template<typename T>
	inline T Asin(T scalar)
	{
		return std::asin(scalar);
	}

	template<typename T>
	inline T Atan(T scalar)
	{
		return std::atan(scalar);
	}

	template<typename T>
	inline T Atan2(T x, T y)
	{
		return std::atan2(x, y);
	}

	template<typename T>
	inline T Cosh(T scalar)
	{
		return std::cosh(scalar);
	}

	template<typename T>
	inline T Sinh(T scalar)
	{
		return std::sinh(scalar);
	}

	template<typename T>
	inline T Tanh(T scalar)
	{
		return std::tanh(scalar);
	}

	template<typename T>
	inline T Acosh(T scalar)
	{
		return std::acosh(scalar);
	}

	template<typename T>
	inline T Asinh(T scalar)
	{
		return std::asinh(scalar);
	}

	template<typename T>
	inline T Atanh(T scalar)
	{
		return std::atanh(scalar);
	}
}
