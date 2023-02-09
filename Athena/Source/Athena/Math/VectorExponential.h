#pragma once

#include "Athena/Math/TypesImpl/Vector.h"
#include "Athena/Math/TypesImpl/Vector4.h"
#include "Exponential.h"


namespace Athena::Math
{
	template <typename T, uint32 Size>
	inline Vector<T, Size> Sqrt(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (uint32 i = 0; i < Size; ++i)
			out[i] = Math::Sqrt(vec[i]);

		return out;
	}

	template <typename T, uint32 Size>
	inline Vector<T, Size> Cbrt(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (uint32 i = 0; i < Size; ++i)
			out[i] = Math::Cbrt(vec[i]);

		return out;
	}

	template <typename T, uint32 Size>
	inline Vector<T, Size> ExpE(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (uint32 i = 0; i < Size; ++i)
			out[i] = Math::ExpE(vec[i]);

		return out;
	}

	template <typename T, uint32 Size>
	inline Vector<T, Size> Exp2(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (uint32 i = 0; i < Size; ++i)
			out[i] = Math::Exp2(vec[i]);

		return out;
	}

	template <typename T, uint32 Size>
	inline Vector<T, Size> LogE(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (uint32 i = 0; i < Size; ++i)
			out[i] = Math::LogE(vec[i]);

		return out;
	}

	template <typename T, uint32 Size>
	inline Vector<T, Size> Log2(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (uint32 i = 0; i < Size; ++i)
			out[i] = Math::Log2(vec[i]);

		return out;
	}

	template <typename T, uint32 Size>
	inline Vector<T, Size> Log10(const Vector<T, Size>& vec)
	{
		Vector<T, Size> out;
		for (uint32 i = 0; i < Size; ++i)
			out[i] = Math::Log10(vec[i]);

		return out;
	}

	template <typename T, uint32 Size>
	inline Vector<T, Size> Pow(const Vector<T, Size>& vec, T exp)
	{
		Vector<T, Size> out;
		for (uint32 i = 0; i < Size; ++i)
			out[i] = Math::Pow(vec[i], exp);

		return out;
	}
}

#ifdef ATN_SIMD
#include "Athena/Math/SIMD/VectorExponential_simd.h"
#endif
