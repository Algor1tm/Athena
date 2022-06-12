#pragma once


#define M_PI 3.14159265358979323846
#define M_PIf 3.14159265358979323846f


namespace Athena
{
	constexpr float DegreeToRad(int degree)
	{
		return float(degree) * M_PIf / 180.f;
	}

	constexpr float DegreeToRad(float degree)
	{
		return degree * M_PIf / 180.f;
	}

	constexpr double DegreeToRad(double degree)
	{
		return degree * M_PI / 180.;
	}


	constexpr float RadToDegree(float radians)
	{
		return radians * 180.f / M_PIf;
	}

	constexpr double RadToDegree(double radians)
	{
		return radians * 180. / M_PI;
	}
	 

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
