#pragma once

#include "Athena/Math/Types/Vector.h"
#include "Trigonometric.h"


namespace Athena::Math
{
	template <typename T, SIZE_T Size>
	inline Vector<T, Size> Radians(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Radians(vec[i]);

		return out;
	}

	template <typename T, SIZE_T Size>
	inline Vector<T, Size> Degrees(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Degrees(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Cos(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Cos(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Sin(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Sin(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Tan(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Tan(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Acos(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Acos(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Asin(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Asin(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Atan(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Atan(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Atan2(const Vector<T, Size>& x, const Vector<T, Size>& y)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Atan2(x[i], y[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Cosh(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Cosh(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Sinh(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Sinh(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Tanh(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Tanh(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Acosh(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Acosh(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Asinh(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Asinh(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Atanh(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Atanh(vec[i]);

		return out;
	}
}
