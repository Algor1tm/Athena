#pragma once

#include "Athena/Core.h"


#define M_PI 3.14159265358979323846
#define M_PIf 3.14159265358979323846f


namespace Athena
{

	constexpr float ATHENA_API DegreeToRad(float degree);
	constexpr double ATHENA_API DegreeToRad(double degree);

	constexpr float ATHENA_API RadToDegree(float radians);
	constexpr double ATHENA_API RadToDegree(double radians);
	 

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
