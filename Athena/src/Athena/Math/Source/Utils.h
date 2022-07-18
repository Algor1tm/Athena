#pragma once

#include "Vector2.h"
#include "Vector3.h"
#include "Athena/Math/SIMD/Vector4_float.h"

#define M_PI 3.14159265358979323846
#define M_PIf 3.14159265358979323846f


namespace Athena
{
	template <typename T>
	constexpr T PI()
	{
		return static_cast<T>(M_PIf);
	}

	template<>
	constexpr float PI<float>()
	{
		return M_PIf;
	}

	template <>
	constexpr double PI<double>()
	{
		return M_PI;
	}

	constexpr float DegreeToRad(int degree)
	{
		return float(degree) * PI<float>() / 180.f;
	}

	constexpr float DegreeToRad(float degree)
	{
		return degree * PI<float>() / 180.f;
	}

	constexpr double DegreeToRad(double degree)
	{
		return degree * PI<double>() / 180.;
	}


	constexpr float RadToDegree(float radians)
	{
		return radians * 180.f / PI<float>();
	}

	constexpr double RadToDegree(double radians)
	{
		return radians * 180. / PI<double>();
	}
	

	template <typename X, typename Y, typename Z>
	constexpr X Clamp(X value, Y min, Z max)
	{
		if (value < static_cast<X>(min))
			return static_cast<X>(min);
		else if (value > static_cast<X>(max))
			return static_cast<X>(max);
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

		static float Float(float min, float max)
		{
			return Lerp(min, max, Float());
		}

		static uint32_t UInt()
		{
			return s_Distribution(s_RandomEngine);
		}

		static uint32_t UInt(uint32_t min, uint32_t max)
		{
			return min + (s_Distribution(s_RandomEngine) % (max - min + 1));
		}

		static Vector<float, 2> Vector2()
		{
			return Vector<float, 2>(Float(), Float());
		}

		static Vector<float, 3> Vector3()
		{
			return Vector<float, 3>(Float(), Float(), Float());
		}

		static Vector<float, 4> Vector4()
		{
			return Vector<float, 4>(Float(), Float(), Float(), Float());
		}

		static Vector<float, 2> Vector2(float min, float max)
		{
			return Vector<float, 2>(Float(min, max), Float(min, max));
		}

		static Vector<float, 3> Vector3(float min, float max)
		{
			return Vector<float, 3>(Float(min, max), Float(min, max), Float(min, max));
		}

		static Vector<float, 4> Vector4(float min, float max)
		{
			return Vector<float, 4>(Float(min, max), Float(min, max), Float(min, max), Float(min, max));
		}

	private:
		static std::mt19937 s_RandomEngine;
		static std::uniform_int_distribution<std::mt19937::result_type> s_Distribution;
	};
#define max(a, b) (((a) > (b)) ? (a) : (b))
}
