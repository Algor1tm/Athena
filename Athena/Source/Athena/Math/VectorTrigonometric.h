#pragma once

#include "Athena/Math/TypesImpl/Vector.h"
#include "Trigonometric.h"


namespace Athena::Math
{
	template <typename T, SIZE_T Size>
	inline Vector<T, Size> Radians(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Radians(vec[i]);

		return out;
	}

	template <typename T, SIZE_T Size>
	inline Vector<T, Size> Degrees(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Degrees(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Cos(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Cos(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Sin(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Sin(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Tan(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Tan(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Acos(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Acos(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Asin(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Asin(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Atan(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Atan(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Atan2(const Vector<T, Size>& x, const Vector<T, Size>& y)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Atan2(x[i], y[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Cosh(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Cosh(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Sinh(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Sinh(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Tanh(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Tanh(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Acosh(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Acosh(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Asinh(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Asinh(vec[i]);

		return out;
	}

	template<typename T, SIZE_T Size>
	inline Vector<T, Size> Atanh(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Math::Atanh(vec[i]);

		return out;
	}
}
