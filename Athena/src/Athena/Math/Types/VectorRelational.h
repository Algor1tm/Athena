#pragma once

#include "Vector.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Athena/Math/Utils.h"

#include <string>


namespace Athena
{
	template <typename T, SIZE_T Size>
	constexpr Vector<T, Size> operator+(T scalar, const Vector<T, Size>& vec)
	{
		return vec + scalar;
	}

	template <typename T, SIZE_T Size>
	constexpr Vector<T, Size> operator*(T scalar, const Vector<T, Size>& vec)
	{
		return vec * scalar;
	}

	template <typename T, SIZE_T Size>
	constexpr void Fill(Vector<T, Size>& vec, T value)
	{
		vec.Fill(value);
	}

	template <typename T, SIZE_T Size>
	constexpr T* Data(const Vector<T, Size>& vec)
	{
		return vec.Data();
	}

	template <typename T, SIZE_T Size>
	constexpr const T* Data(Vector<T, Size>& vec)
	{
		return vec.Data();
	}

	template <typename T, SIZE_T Size>
	constexpr float Distance(const Vector<T, Size>& Left, const Vector<T, Size>& Right)
	{
		return (Left - Right).GetLength();
	}

	template <typename T>
	constexpr T Dot(const Vector<T, 2>& Left, const Vector<T, 2>& Right)
	{
		return Left.x * Right.x + Left.y * Right.y;
	}

	template <typename T>
	constexpr T Dot(const Vector<T, 3>& Left, const Vector<T, 3>& Right)
	{
		return Left.x * Right.x + Left.y * Right.y + Left.z * Right.z;
	}

	template <typename T>
	constexpr T Dot(const Vector<T, 4>& Left, const Vector<T, 4>& Right)
	{
		return Left.x * Right.x + Left.y * Right.y + Left.z * Right.z + Left.w * Right.w;
	}

	template <typename T, SIZE_T Size>
	constexpr T Dot(const Vector<T, Size>& Left, const Vector<T, Size>& Right)
	{
		T out;
		for (SIZE_T i = 0; i < Size; ++i)
			out += Left[i] * Right[i];
		return out;
	}

	template <typename T, SIZE_T Size>
	constexpr T SqrLength(const Vector<T, Size>& vec)
	{
		return vec.SqrLength();
	}

	template <typename T, SIZE_T Size>
	constexpr float Length(const Vector<T, Size>& vec)
	{
		return vec.Length();
	}

	template <typename T, SIZE_T Size>
	constexpr Vector<T, Size>& Normalize(Vector<T, Size>& vec)
	{
		return vec.Normalize();
	}

	template <typename T>
	constexpr T Cross(const Vector<T, 2>& Left, const Vector<T, 2>& Right)
	{
		return Left.x * Right.y - Left.y * Right.x;
	}

	template <typename T>
	constexpr Vector<T, 3> Cross(const Vector<T, 3>& Left, const Vector<T, 3>& Right)
	{
		Vector3 out;
		out.x = Left.y * Right.z - Left.z * Right.y;
		out.y = -Left.x * Right.z + Left.z * Right.x;
		out.z = Left.x * Right.y - Left.y * Right.x;
		return out;
	}

	/// <param name="Normal">: Must be normalized</param>
	template <typename T>
	constexpr Vector<T, 2> Reflect(
		const Vector<T, 2>& Direction, const Vector<T, 2>& Normal)
	{
		return Direction - 2 * Normal * Dot(Direction, Normal);
	}

	/// <param name="Normal">: Must be normalized</param>
	template <typename T>
	constexpr Vector<T, 3> Reflect(
		const Vector<T, 3>& Direction, const Vector<T, 3>& Normal)
	{
		return Direction - 2 * Normal * Dot(Direction, Normal);
	}

	template <typename T, SIZE_T Size>
	constexpr void Swap(Vector<T, Size>& Left, Vector<T, Size>& Right)
	{
		for (SIZE_T i = 0; i < Size; ++i)
			std::swap(Left[i], Right[i]);
	}

	// Does not validate input values
	template <typename T, SIZE_T Size>
	constexpr Vector<T, Size> Lerp(
		const Vector<T, Size>& a, const Vector<T, Size>& b, T t)
	{
		Vector<T, Size> out;
		for (SIZE_T i = 0; i < Size; ++i)
			out[i] = Lerp(a[i], b[i], t);

		return out;
	}

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
	constexpr std::string ToString(const Vector<T, Size>& vec)
	{
		std::string out = "Vector" + std::to_string(vec.Size()) + "(";
		for (SIZE_T i = 0; i < vec.Size() - 1; ++i)
			out += std::to_string(vec[i]) + ", ";
		out += std::to_string(vec[vec.Size() - 1]) + ")";
		return out;
	}
}
