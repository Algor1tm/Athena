#pragma once

#include "Vector.h"



namespace Athena
{
#define Size3 3

	template <typename Ty>
	class Vector<Ty, Size3>
	{
	public:
		using iterator = VectorIterator<Ty, Size3>;
		using const_iterator = VectorConstIterator<Ty, Size3>;

	// Constructors
	public:
		constexpr Vector() = default;

		constexpr Vector(Ty value)
			: x(value),
			y(value),
			z(value) {}


		constexpr Vector(Ty _x, Ty _y, Ty _z)
			: x(_x),
			y(_y),
			z(_z) {}


		template<typename X, typename Y, typename Z>
		constexpr Vector(X _x, Y _y, Z _z)
			: x(static_cast<Ty>(_x)),
			y(static_cast<Ty>(_y)),
			z(static_cast<Ty>(_z)) {}


		constexpr Vector(const Vector& other) = default;
		constexpr Vector& operator=(const Vector& other) = default;


		template <typename U>
		constexpr Vector<Ty, Size3>(const Vector<U, Size3>& other)
			: x(static_cast<Ty>(other.x)),
			y(static_cast<Ty>(other.y)),
			z(static_cast<Ty>(other.z)) {}


		template <typename U>
		constexpr Vector<Ty, Size3>& operator=(const Vector<U, Size3>& other)
		{
			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);
			z = static_cast<Ty>(other.z);

			return *this;
		}


		template <typename X, typename Y>
		constexpr Vector<Ty, Size3>(const Vector<X, 2>& _xy, Y _z)
			: x(static_cast<Ty>(_xy.x)),
			y(static_cast<Ty>(_xy.y)),
			z(static_cast<Ty>(z)) {}


		template <typename X, typename Y>
		constexpr Vector<Ty, Size3>(X _x, const Vector<Y, 2>& _yz)
			: x(static_cast<Ty>(_x)),
			y(static_cast<Ty>(_yz.x)),
			z(static_cast<Ty>(_yz.y)) {}


		template <typename U>
		constexpr Vector<Ty, Size3>(const Vector<U, 2>& _xy)
			: x(static_cast<Ty>(_xy.x)),
			y(static_cast<Ty>(_xy.y)),
			z(static_cast<Ty>(1)) {}


		template <typename U>
		constexpr Vector<Ty, Size3>& operator=(const Vector<U, 2>& _xy)
		{
			x = static_cast<Ty>(_xy.x);
			y = static_cast<Ty>(_xy.y);
			z = static_cast<Ty>(1);

			return *this;
		}


		template <typename U>
		constexpr Vector<Ty, Size3>(const Vector<U, 4>& _xyz)
			: x(static_cast<Ty>(_xyz.x)),
			y(static_cast<Ty>(_xyz.y)),
			z(static_cast<Ty>(_xyz.z)) {}


		template <typename U>
		constexpr Vector<Ty, Size3>& operator=(const Vector<U, 4>& _xyz)
		{
			x = static_cast<Ty>(_xyz.x);
			y = static_cast<Ty>(_xyz.y);
			z = static_cast<Ty>(_xyz.z);

			return *this;
		}

	// Public Methods
	public:
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

		constexpr void Fill(Ty value)
		{
			x = value;
			y = value;
			z = value;
		}

		constexpr Vector& Apply(Ty(*func)(Ty))
		{
			x = func(x);
			y = func(y);
			z = func(z);
			return *this;
		}

		constexpr Ty SqrLength() const
		{
			return x * x + y * y + z * z;
		}

		constexpr float Length() const
		{
			return std::sqrt(static_cast<float>(SqrLength()));
		}

		constexpr Vector& Normalize()
		{
			float length = Length();
			return length == 0 ? *this : *this /= static_cast<Ty>(length);
		}

		constexpr Vector GetNormalized() const
		{
			float length = Length();
			return length == 0 ? Vector(*this) : Vector(*this) /= static_cast<Ty>(length);
		}

	// Operators
	public:
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

	// Static Methods
	public:
		static Vector up()
		{
			return Vector(static_cast<Ty>(0), static_cast<Ty>(1), static_cast<Ty>(0));
		}

		static Vector down()
		{
			return Vector(static_cast<Ty>(0), static_cast<Ty>(-1), static_cast<Ty>(0));
		}

		static Vector left()
		{
			return Vector(static_cast<Ty>(-1), static_cast<Ty>(0), static_cast<Ty>(0));
		}

		static Vector right()
		{
			return Vector(static_cast<Ty>(1), static_cast<Ty>(0), static_cast<Ty>(0));
		}

		static Vector forward()
		{
			return Vector(static_cast<Ty>(0), static_cast<Ty>(0), static_cast<Ty>(-1));
		}

		static Vector back()
		{
			return Vector(static_cast<Ty>(0), static_cast<Ty>(0), static_cast<Ty>(1));
		}

	public:
		Ty x, y, z;
	};

#undef Size3
}
