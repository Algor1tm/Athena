#pragma once

#include "Vector.h"



namespace Athena
{
#define Size3 3

#pragma pack(push, 1)
	template <typename Ty>
	class Vector<Ty, Size3>
	{
	public:
		using iterator = VectorIterator<Ty, Size3>;
		using const_iterator = VectorConstIterator<Ty, Size3>;

	public:
		Vector() = default;

		constexpr Vector(Ty value)
			: x(value), y(value), z(value) {}

		constexpr Vector(Ty X, Ty Y, Ty Z)
			: x(X), y(Y), z(Z) {}

		constexpr Vector(const Vector& other) = default;

		template <typename U>
		constexpr Vector<Ty, Size3>(const Vector<U, Size3>& other)
		{
			static_assert(std::is_convertible<U, Ty>::value,
				"Vector initialization error: Vectors are not convertible");

			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);
			z = static_cast<Ty>(other.z);
		}

		constexpr Vector& operator=(const Vector& other) = default;

		template <typename U>
		constexpr Vector<Ty, Size3>& operator=(const Vector<U, Size3>& other)
		{
			static_assert(std::is_convertible<U, Ty>::value,
				"Vector assignment error: Vectors are not convertible");

			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);
			z = static_cast<Ty>(other.z);

			return *this;
		}

		constexpr void Fill(Ty value)
		{
			x = value;
			y = value;
			z = value;
		}

		constexpr const Ty& operator[](size_t idx) const
		{
			ATN_CORE_ASSERT(idx < Size3, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr Ty& operator[](size_t idx)
		{
			ATN_CORE_ASSERT(idx < Size3, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr size_t Size() const
		{
			return Size3;
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
			return iterator(&x, Size3);
		}

		constexpr const_iterator cbegin() const 
		{
			return const_iterator(&x, 0);
		}

		constexpr const_iterator cend() const 
		{
			return const_iterator(&x, Size3);
		}

		constexpr Vector& Apply(Ty (*func)(Ty))
		{
			x = func(x);
			y = func(y);
			z = func(z);
			return *this;
		}

		constexpr Ty GetSqrLength() const 
		{
			return x * x + y * y + z * z;
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
			return *this;
		}

		constexpr Vector& operator-=(const Vector& other) 
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}

		constexpr Vector& operator+=(Ty scalar) 
		{
			x += scalar;
			y += scalar;
			z += scalar;
			return *this;
		}

		constexpr Vector& operator-=(Ty scalar) 
		{
			x -= scalar;
			y -= scalar;
			z -= scalar;
			return *this;
		}

		constexpr Vector& operator*=(Ty scalar) 
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}

		constexpr Vector& operator/=(Ty scalar)
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector operation error: dividing by zero");
			x /= scalar;
			y /= scalar;
			z /= scalar;
			return *this;
		}

		constexpr Vector operator+(const Vector& other) const 
		{
			return Vector(x + other.x, y + other.y, z + other.z);
		}

		constexpr Vector operator-(const Vector& other) const 
		{
			return Vector(x - other.x, y - other.y, z - other.z);
		}

		constexpr Vector operator+(Ty scalar) const 
		{
			return Vector(x + scalar, y + scalar, z + scalar);
		}

		constexpr Vector operator-(Ty scalar) const 
		{
			return Vector(x - scalar, y - scalar, z - scalar);
		}

		constexpr Vector operator*(Ty scalar) const 
		{
			return Vector(x * scalar, y * scalar, z * scalar);
		}

		constexpr Vector operator/(Ty scalar) const
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector operation error: dividing by zero");
			return Vector(x / scalar, y / scalar, z / scalar);
		}

		constexpr Vector operator-() const 
		{
			return Vector(-x, -y, -z);
		}

		constexpr bool operator==(const Vector& other) const 
		{
			return x == other.x && y == other.y && z == other.z;
		}

		constexpr bool operator!=(const Vector& other) const 
		{
			return !(*this == other);
		}

	public:
		static const Vector up;
		static const Vector down;
		static const Vector left;
		static const Vector right;
		static const Vector forward;
		static const Vector back;

	public:
		Ty x, y, z;
	};
#pragma pack(pop)

	template<typename Ty>
	const Vector<Ty, Size3> Vector<Ty, Size3>::up =
		Vector<Ty, Size3>(static_cast<Ty>(0), static_cast<Ty>(-1), static_cast<Ty>(0));

	template<typename Ty>
	const Vector<Ty, Size3> Vector<Ty, Size3>::down =
		Vector<Ty, Size3>(static_cast<Ty>(0), static_cast<Ty>(1), static_cast<Ty>(0));

	template<typename Ty>
	const Vector<Ty, Size3> Vector<Ty, Size3>::left =
		Vector<Ty, Size3>(static_cast<Ty>(-1), static_cast<Ty>(0), static_cast<Ty>(0));

	template<typename Ty>
	const Vector<Ty, Size3> Vector<Ty, Size3>::right =
		Vector<Ty, Size3>(static_cast<Ty>(1), static_cast<Ty>(0), static_cast<Ty>(0));

	template<typename Ty>
	const Vector<Ty, Size3> Vector<Ty, Size3>::forward =
		Vector<Ty, Size3>(static_cast<Ty>(0), static_cast<Ty>(0), static_cast<Ty>(-1));

	template<typename Ty>
	const Vector<Ty, Size3> Vector<Ty, Size3>::back =
		Vector<Ty, Size3>(static_cast<Ty>(0), static_cast<Ty>(0), static_cast<Ty>(1));

#undef Size3
}
