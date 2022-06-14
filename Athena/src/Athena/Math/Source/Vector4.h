#pragma once

#include "Vector.h"


namespace Athena
{
#define Size4 4

#pragma pack(push, 1)
	template <typename Ty>
	class Vector<Ty, Size4>
	{
	public:
		using iterator = VectorIterator<Ty, Size4>;
		using const_iterator = VectorConstIterator<Ty, Size4>;

	public:
		Vector() = default;

		constexpr Vector(Ty value)
			: x(value), y(value), z(value), w(value) {}

		constexpr Vector(Ty X, Ty Y, Ty Z, Ty W)
			: x(X), y(Y), z(Z), w(W) {}

		constexpr Vector(const Vector& other) = default;

		constexpr Vector<Ty, Size4>(const Vector<Ty, 3>& other)
			: Vector(other.x, other.y, other.z, static_cast<Ty>(1)) {}

		template <typename U>
		constexpr Vector<Ty, Size4>(const Vector<U, 3>& other)
		{
			static_assert(std::is_convertible<U, Ty>::value,
				"Vector initialization error: Vectors are not convertible");

			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);
			z = static_cast<Ty>(other.z);
			w = static_cast<Ty>(1);
		}

		template <typename U>
		constexpr Vector<Ty, Size4>(const Vector<U, Size4>& other)
		{
			static_assert(std::is_convertible<U, Ty>::value,
				"Vector initialization error: Vectors are not convertible");

			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);
			z = static_cast<Ty>(other.z);
			w = static_cast<Ty>(other.w);
		}

		constexpr Vector& operator=(const Vector& other) = default;

		template <typename U>
		constexpr Vector<Ty, Size4>& operator=(const Vector<U, Size4>& other)
		{
			static_assert(std::is_convertible<U, Ty>::value,
				"Vector assignment error: Vectors are not convertible");

			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);
			z = static_cast<Ty>(other.z);
			w = static_cast<Ty>(other.w);

			return *this;
		}

		constexpr void Fill(Ty value)
		{
			x = value;
			y = value;
			z = value;
			w = value;
		}

		constexpr const Ty& operator[](size_t idx) const
		{
			ATN_CORE_ASSERT(idx < Size4, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr Ty& operator[](size_t idx)
		{
			ATN_CORE_ASSERT(idx < Size4, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr size_t Size() const
		{
			return Size4;
		}

		constexpr Ty* Data()
		{
			return &x;
		}

		constexpr const Ty* Data() const
		{
			return &x;
		}

		constexpr iterator begin()
		{
			return iterator(&x, 0);
		}

		constexpr iterator end()
		{
			return iterator(&x, Size4);
		}

		constexpr const_iterator cbegin() const
		{
			return const_iterator(&x, 0);
		}

		constexpr const_iterator cend() const
		{
			return const_iterator(&x, Size4);
		}

		constexpr Vector& Apply(Ty (*func)(Ty))
		{
			x = func(x);
			y = func(y);
			z = func(z);
			w = func(w);
			return *this;
		}

		constexpr Ty GetSqrLength() const 
		{
			return x * x + y * y + z * z + w * w;
		}

		constexpr float GetLength() const 
		{
			return std::sqrt(static_cast<float>(GetSqrLength()));
		}

		constexpr Vector& Normalize()
		{
			float length = GetLength();
			return length == 0 ? *this : *this /= static_cast<Ty>(length);
		}

		constexpr Vector GetNormalized() const
		{
			float length = GetLength();
			return length == 0 ? Vector(*this) : Vector(*this) /= static_cast<Ty>(length);
		}

	public:
		constexpr Vector& operator+=(const Vector& other) 
		{
			x += other.x;
			y += other.y;
			z += other.z;
			w += other.w;
			return *this;
		}

		constexpr Vector& operator-=(const Vector& other) 
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			w -= other.w;
			return *this;
		}

		constexpr Vector& operator+=(Ty scalar) 
		{
			x += scalar;
			y += scalar;
			z += scalar;
			w += scalar;
			return *this;
		}

		constexpr Vector& operator-=(Ty scalar) 
		{
			x -= scalar;
			y -= scalar;
			z -= scalar;
			w -= scalar;
			return *this;
		}

		constexpr Vector& operator*=(Ty scalar) 
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			w *= scalar;
			return *this;
		}

		constexpr Vector& operator/=(Ty scalar)
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector operation error: dividing by zero");
			x /= scalar;
			y /= scalar;
			z /= scalar;
			w /= scalar;
			return *this;
		}

		constexpr Vector operator+(const Vector& other) const 
		{
			return Vector(x + other.x, y + other.y, z + other.z, w + other.w);
		}

		constexpr Vector operator-(const Vector& other) const 
		{
			return Vector(x - other.x, y - other.y, z - other.z, w - other.w);
		}

		constexpr Vector operator+(Ty scalar) const 
		{
			return Vector(x + scalar, y + scalar, z + scalar, w + scalar);
		}

		constexpr Vector operator-(Ty scalar) const 
		{
			return Vector(x - scalar, y - scalar, z - scalar, w - scalar);
		}

		constexpr Vector operator*(Ty scalar) const 
		{
			return Vector(x * scalar, y * scalar, z * scalar, w * scalar);
		}

		constexpr Vector operator/(Ty scalar) const
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector operation error: dividing by zero");
			return Vector(x / scalar, y / scalar, z / scalar, w / scalar);
		}

		constexpr Vector operator-() const 
		{
			return Vector(-x, -y, -z, -w);
		}

		constexpr bool operator==(const Vector& other) const 
		{
			return x == other.x && y == other.y && z == other.z && w == other.w;
		}

		constexpr bool operator!=(const Vector& other) const 
		{
			return !(*this == other);
		}

	public:
		Ty x, y, z, w;
	};
#pragma pack(pop)
#undef Size4
}
