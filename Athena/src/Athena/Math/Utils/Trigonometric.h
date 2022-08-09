#pragma once


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
}
