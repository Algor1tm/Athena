#pragma once

#include <limits>
#include <cmath>

#ifdef max
	#undef max
#endif

#ifdef min
	#undef min
#endif

namespace Athena::Math
{
	template <typename T>
	constexpr T Epsilon()
	{
		return std::numeric_limits<T>::epsilon();
	}

	template <typename T>
	constexpr T Infinity()
	{
		return std::numeric_limits<T>::infinity();
	}

	template <typename T>
	constexpr T MaxValue()
	{
		return std::numeric_limits<T>::max();
	}
	 
	template<typename T>
	constexpr T MinValue()
	{
		return std::numeric_limits<T>::lowest();
	}

	template <typename T>
	constexpr T NaN()
	{
		return std::numeric_limits<T>::quiet_NaN();
	}

	template <typename T>
	inline bool IsNaN(T value)
	{
		return std::isnan(value);
	}
}
