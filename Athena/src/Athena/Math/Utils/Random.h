#pragma once

#include "Athena/Math/Vector.h"
#include "Athena/Math/SIMD/Types/Vector4_float.h"

#include <random>


namespace Athena
{
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
			return (float)s_Distribution(s_RandomEngine) / (float)std::numeric_limits<uint32>::max();
		}

		static float Float(float min, float max)
		{
			return Lerp(min, max, Float());
		}

		static uint32 UInt()
		{
			return s_Distribution(s_RandomEngine);
		}

		static uint32 UInt(uint32 min, uint32 max)
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
