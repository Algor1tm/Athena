#pragma once

#include "Vector2.h"

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
	

	template <typename T>
	constexpr T Clamp(T value, T min, T max)
	{
		if (value < min)
			return min;
		else if (value > max)
			return max;
		return value;
	}

	// Does not validate input values
	template <typename T>
	constexpr T Lerp(T a, T b, T t)
	{
		return a + (b - a) * t;
	}

	// Does not validate input values
	template <typename T>
	constexpr T InverseLerp(T lerp, T a, T b)
	{
		return (lerp - a) / (b - a);
	}


	enum class Orienation
	{
		Collinear = 0, ClockWise, CounterClockWise
	};

	template <typename Ty>
	constexpr Orienation Orientate(const Vector<Ty, 2>& p, const Vector<Ty, 2>& q, const Vector<Ty, 2>& r)
	{
		Ty val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);

		if (val == 0) return Orienation::Collinear;
		return (val > 0) ? Orienation::ClockWise : Orienation::CounterClockWise;
	}


#undef max
	class Random
	{
	public:
		static void Init()
		{
			s_RandomEngine.seed(std::random_device()());
		}

		static float Float()
		{
			return (float)s_Distribution(s_RandomEngine) / (float)std::numeric_limits<uint32_t>::max();
		}

	private:
		static std::mt19937 s_RandomEngine;
		static std::uniform_int_distribution<std::mt19937::result_type> s_Distribution;
	};
#define max(a, b) (((a) > (b)) ? (a) : (b))
}
