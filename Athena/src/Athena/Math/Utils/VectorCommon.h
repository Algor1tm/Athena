#pragma once

#include "Athena/Math/Types/Vector.h"
#include "Common.h"


namespace Athena
{
	template<typename X, SIZE_T Size, typename Y, typename Z>
	constexpr Vector<X, Size> Clamp(const Vector<X, Size>& vec, Y min, Z max)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Clamp(vec[i], static_cast<X>(min), static_cast<X>(max));

		return out;
	}

	template<typename T, SIZE_T Size>
	constexpr Vector<T, Size> Clamp(const Vector<T, Size>& vec, const Vector<T, Size>& min, const Vector<T, Size>& max)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Clamp(vec[i], min[i], max[i]);

		return out;
	}

	template <typename T, SIZE_T Size>
	constexpr Vector<T, Size> Lerp(
		const Vector<T, Size>& a, const Vector<T, Size>& b, T t)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Lerp(a[i], b[i], t);

		return out;
	}




	template<typename T, SIZE_T Size>
	constexpr Vector<T, Size> Abs(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Abs(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	constexpr Vector<T, Size> Sign(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Sign(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	constexpr bool All(const Vector<T, Size>& vec)
	{
		for (SIZE_T i = 0; i < Size; ++i)
		{
			if (static_cast<bool>(vec[i]) == false)
				return false;
		}

		return true;
	}

	template<typename T, SIZE_T Size>
	constexpr bool Any(const Vector<T, Size>& vec)
	{
		for (SIZE_T i = 0; i < Size; ++i)
		{
			if (static_cast<bool>(vec[i]) == true)
				return true;
		}

		return false;
	}



	template<typename T, SIZE_T Size>
	constexpr Vector<T, Size> Round(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Round(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	constexpr Vector<T, Size> Floor(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Floor(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	constexpr Vector<T, Size> Ceil(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Ceil(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	constexpr Vector<T, Size> Trunc(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Trunc(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	constexpr Vector<T, Size> Fract(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Fract(vec[i]);

		return out;
	}



	template<typename T, SIZE_T Size>
	constexpr Vector<T, Size> Max(const Vector<T, Size>& left, const Vector<T, Size>& right)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Max(left[i], right[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	constexpr  Vector<T, Size> Min(const Vector<T, Size>& left, const Vector<T, Size>& right)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Min(left[i], right[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	constexpr Vector<T, Size> Max(const Vector<T, Size>& vec)
	{
		T result = vec[0];
		for (SIZE_T i = 1; i < Size; ++i)
		{
			if (result < vec[i])
				result = vec[i];
		}

		return result;
	}

	template<typename T, SIZE_T Size>
	constexpr  Vector<T, Size> Min(const Vector<T, Size>& vec)
	{
		T result = vec[0];
		for (SIZE_T i = 1; i < Size; ++i)
		{
			if (result > vec[i])
				result = vec[i];
		}

		return result;
	}

	template <typename T, SIZE_T Size>
	constexpr void Swap(Vector<T, Size>& left, Vector<T, Size>& right)
	{
		for (SIZE_T i = 0; i < Size; ++i)
			Swap(left[i], right[i]);
	}
}


#ifdef ATN_SIMD
#include "Athena/Math/SIMD/Utils/VectorCommon_simd.h"
#endif
