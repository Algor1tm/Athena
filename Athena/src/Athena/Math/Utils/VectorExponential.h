#pragma once

#include "Athena/Math/Types/Vector.h"
#include "Exponential.h"


namespace Athena
{
	template <typename T, SIZE_T Size>
	inline Vector<T, Size> Sqrt(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Sqrt(vec[i]);

		return out;
	}

	template <typename T, SIZE_T Size>
	inline Vector<T, Size> Cbrt(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Cbrt(vec[i]);

		return out;
	}

	template <typename T, SIZE_T Size>
	inline Vector<T, Size> ExpE(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = ExpE(vec[i]);

		return out;
	}

	template <typename T, SIZE_T Size>
	inline Vector<T, Size> Exp2(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Exp2(vec[i]);

		return out;
	}

	template <typename T, SIZE_T Size>
	inline Vector<T, Size> LogE(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = LogE(vec[i]);

		return out;
	}

	template <typename T, SIZE_T Size>
	inline Vector<T, Size> Log2(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Log2(vec[i]);

		return out;
	}

	template <typename T, SIZE_T Size>
	inline Vector<T, Size> Log10(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Log10(vec[i]);

		return out;
	}

	template <typename T, SIZE_T Size>
	inline Vector<T, Size> Pow(const Vector<T, Size>& vec, T exp)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Pow(vec[i], exp);

		return out;
	}
}

#ifdef ATN_SIMD
#include "Athena/Math/SIMD/Utils/VectorExponential_simd.h"
#endif
