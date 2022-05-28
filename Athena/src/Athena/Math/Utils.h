#pragma once

#define M_PI 3.14159265358979323846
#define M_PIf 3.14159265358979323846f


namespace Athena
{

	constexpr float DegreeToRad(float degree);
	constexpr double DegreeToRad(double degree);

	constexpr float RadToDegree(float radians);
	constexpr double RadToDegree(double radians);
	 

	// Does not validate input values
	template <typename T>
	constexpr T Lerp(T min, T max, T mid)
	{
		return min + (max - min) * mid;
	}

	// Does not validate input values
	template <typename T>
	constexpr T InverseLerp(T lerp, T min, T max)
	{
		return (lerp - min) / (max - min);
	}
}
