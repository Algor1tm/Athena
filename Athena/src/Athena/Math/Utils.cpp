#include "atnpch.h"
#include "Utils.h"


namespace Athena
{
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
}