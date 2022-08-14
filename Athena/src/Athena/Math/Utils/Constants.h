#pragma once



namespace Athena
{
	template <typename T>
	constexpr T PI()
	{
		return static_cast<T>(3.14159265358979323846);
	}

	template <typename T>
	constexpr T E()
	{
		return static_cast<T>(2.71828182845904523536);
	}

	template <typename T>
	constexpr T RootTwo()
	{
		return static_cast<T>(1.414213562373095048801);
	}

	template <typename T>
	constexpr T RootThree()
	{
		return static_cast<T>(1.7320508075688772935274);
	}

	template <typename T>
	constexpr T RootFive()
	{
		return static_cast<T>(2.23606797749978969640917);
	}

	template <typename T>
	constexpr T GoldenRatio()
	{
		return static_cast<T>(1.618033988749894848204586);
	}
}
