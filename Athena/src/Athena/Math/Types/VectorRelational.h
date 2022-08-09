#pragma once

#include "Vector.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

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
	constexpr float Distance(const Vector<T, Size>& left, const Vector<T, Size>& right)
	{
		return (left - right).GetLength();
	}

	template <typename T>
	constexpr T Dot(const Vector<T, 2>& left, const Vector<T, 2>& right)
	{
		return left.x * right.x + left.y * right.y;
	}

	template <typename T>
	constexpr T Dot(const Vector<T, 3>& left, const Vector<T, 3>& right)
	{
		return left.x * right.x + left.y * right.y + left.z * right.z;
	}

	template <typename T>
	constexpr T Dot(const Vector<T, 4>& left, const Vector<T, 4>& right)
	{
		return left.x * right.x + left.y * right.y + left.z * right.z + left.w * right.w;
	}

	template <typename T, SIZE_T Size>
	constexpr T Dot(const Vector<T, Size>& left, const Vector<T, Size>& right)
	{
		T out;
		for (SIZE_T i = 0; i < Size; ++i)
			out += left[i] * right[i];
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
	constexpr T Cross(const Vector<T, 2>& left, const Vector<T, 2>& right)
	{
		return left.x * right.y - left.y * right.x;
	}

	template <typename T>
	constexpr Vector<T, 3> Cross(const Vector<T, 3>& left, const Vector<T, 3>& right)
	{
		Vector3 out;
		out.x = left.y * right.z - left.z * right.y;
		out.y = -left.x * right.z + left.z * right.x;
		out.z = left.x * right.y - left.y * right.x;
		return out;
	}

	/// <param name="Normal">: Must be normalized</param>
	template <typename T>
	constexpr Vector<T, 2> Reflect(
		const Vector<T, 2>& direction, const Vector<T, 2>& normal)
	{
		return direction - 2 * normal * Dot(direction, normal);
	}

	/// <param name="Normal">: Must be normalized</param>
	template <typename T>
	constexpr Vector<T, 3> Reflect(
		const Vector<T, 3>& direction, const Vector<T, 3>& normal)
	{
		return direction - 2 * normal * Dot(direction, normal);
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
