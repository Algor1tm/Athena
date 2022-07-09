#pragma once

#include "Vector.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Utils.h"


namespace Athena
{
	template <typename Ty, size_t Size>
	constexpr Vector<Ty, Size> operator+(Ty scalar, const Vector<Ty, Size>& vec)
	{
		return vec + scalar;
	}

	template <typename Ty, size_t Size>
	constexpr Vector<Ty, Size> operator*(Ty scalar, const Vector<Ty, Size>& vec)
	{
		return vec * scalar;
	}

	template <typename Ty, size_t Size>
	constexpr void Fill(Vector<Ty, Size>& vec, Ty value)
	{
		vec.Fill(value);
	}

	template <typename Ty, size_t Size>
	constexpr Ty* Data(const Vector<Ty, Size>& vec)
	{
		return vec.Data();
	}

	template <typename Ty, size_t Size>
	constexpr Ty* Data(Vector<Ty, Size>& vec)
	{
		return vec.Data();
	}

	template <typename Ty, size_t Size>
	constexpr float Distance(const Vector<Ty, Size>& Left, const Vector<Ty, Size>& Right)
	{
		return (Left - Right).GetLength();
	}

	template <typename Ty>
	constexpr Ty Dot(const Vector<Ty, 2>& Left, const Vector<Ty, 2>& Right)
	{
		return Left.x * Right.x + Left.y * Right.y;
	}

	template <typename Ty>
	constexpr Ty Dot(const Vector<Ty, 3>& Left, const Vector<Ty, 3>& Right)
	{
		return Left.x * Right.x + Left.y * Right.y + Left.z * Right.z;
	}

	template <typename Ty>
	constexpr Ty Dot(const Vector<Ty, 4>& Left, const Vector<Ty, 4>& Right)
	{
		return Left.x * Right.x + Left.y * Right.y + Left.z * Right.z + Left.w * Right.w;
	}

	template <typename Ty, size_t Size>
	constexpr Ty Dot(const Vector<Ty, Size>& Left, const Vector<Ty, Size>& Right)
	{
		Ty out;
		for (size_t i = 0; i < Size; ++i)
			out += Left[i] * Right[i];
		return out;
	}

	template <typename Ty, size_t Size>
	constexpr Ty SqrLength(const Vector<Ty, Size>& vec)
	{
		return vec.SqrLength();
	}

	template <typename Ty, size_t Size>
	constexpr float Length(const Vector<Ty, Size>& vec)
	{
		return vec.Length();
	}

	template <typename Ty, size_t Size>
	constexpr Vector<Ty, Size>& Normalize(Vector<Ty, Size>& vec)
	{
		return vec.Normalize();
	}

	template <typename Ty>
	constexpr Ty Cross(const Vector<Ty, 2>& Left, const Vector<Ty, 2>& Right)
	{
		return Left.x * Right.y - Left.y * Right.x;
	}

	template <typename Ty>
	constexpr Vector<Ty, 3> Cross(const Vector<Ty, 3>& Left, const Vector<Ty, 3>& Right)
	{
		Vector3 out;
		out.x = Left.y * Right.z - Left.z * Right.y;
		out.y = -Left.x * Right.z + Left.z * Right.x;
		out.z = Left.x * Right.y - Left.y * Right.x;
		return out;
	}

	/// <param name="Normal">: Must be normalized</param>
	template <typename Ty>
	constexpr Vector<Ty, 2> Reflect(
		const Vector<Ty, 2>& Direction, const Vector<Ty, 2>& Normal)
	{
		return Direction - 2 * Normal * Dot(Direction, Normal);
	}

	/// <param name="Normal">: Must be normalized</param>
	template <typename Ty>
	constexpr Vector<Ty, 3> Reflect(
		const Vector<Ty, 3>& Direction, const Vector<Ty, 3>& Normal)
	{
		return Direction - 2 * Normal * Dot(Direction, Normal);
	}

	template <typename Ty, size_t Size>
	constexpr void Swap(Vector<Ty, Size>& Left, Vector<Ty, Size>& Right)
	{
		for (size_t i = 0; i < Size; ++i)
			std::swap(Left[i], Right[i]);
	}

	// Does not validate input values
	template <typename Ty, size_t Size>
	constexpr Vector<Ty, Size> Lerp(
		const Vector<Ty, Size>& a, const Vector<Ty, Size>& b, Ty t)
	{
		Vector<Ty, Size> out;
		for (size_t i = 0; i < Size; ++i)
			out[i] = Lerp(a[i], b[i], t);
	}

	template <typename Ty, size_t Size>
	constexpr std::string ToString(const Vector<Ty, Size>& vec)
	{
		std::string out = "Vector" + std::to_string(vec.GetSize()) + "(";
		for (size_t i = 0; i < vec.Size() - 1; ++i)
			out += std::to_string(vec[i]) + ", ";
		out += std::to_string(vec[vec.Size() - 1]) + ")";
		return out;
	}
}
