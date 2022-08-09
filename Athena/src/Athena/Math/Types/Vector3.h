#pragma once

#include "Vector.h"



namespace Athena
{
#define Size3 3

	template <typename T>
	class Vector<T, Size3>
	{
	public:
		using iterator = VectorIterator<T, Size3>;
		using const_iterator = VectorConstIterator<T, Size3>;

	// Constructors
	public:
		constexpr Vector() = default;

		constexpr Vector(T value)
			: x(value),
			y(value),
			z(value) {}


		constexpr Vector(T _x, T _y, T _z)
			: x(_x),
			y(_y),
			z(_z) {}


		template<typename X, typename Y, typename Z>
		constexpr Vector(X _x, Y _y, Z _z)
			: x(static_cast<T>(_x)),
			y(static_cast<T>(_y)),
			z(static_cast<T>(_z)) {}


		constexpr Vector(const Vector& other) = default;
		constexpr Vector& operator=(const Vector& other) = default;


		template <typename U>
		constexpr Vector<T, Size3>(const Vector<U, Size3>& other)
			: x(static_cast<T>(other.x)),
			y(static_cast<T>(other.y)),
			z(static_cast<T>(other.z)) {}


		template <typename U>
		constexpr Vector<T, Size3>& operator=(const Vector<U, Size3>& other)
		{
			x = static_cast<T>(other.x);
			y = static_cast<T>(other.y);
			z = static_cast<T>(other.z);

			return *this;
		}


		template <typename X, typename Y>
		constexpr Vector<T, Size3>(const Vector<X, 2>& _xy, Y _z)
			: x(static_cast<T>(_xy.x)),
			y(static_cast<T>(_xy.y)),
			z(static_cast<T>(z)) {}


		template <typename X, typename Y>
		constexpr Vector<T, Size3>(X _x, const Vector<Y, 2>& _yz)
			: x(static_cast<T>(_x)),
			y(static_cast<T>(_yz.x)),
			z(static_cast<T>(_yz.y)) {}


		template <typename U>
		constexpr Vector<T, Size3>(const Vector<U, 2>& _xy)
			: x(static_cast<T>(_xy.x)),
			y(static_cast<T>(_xy.y)),
			z(static_cast<T>(1)) {}


		template <typename U>
		constexpr Vector<T, Size3>& operator=(const Vector<U, 2>& _xy)
		{
			x = static_cast<T>(_xy.x);
			y = static_cast<T>(_xy.y);
			z = static_cast<T>(1);

			return *this;
		}


		template <typename U>
		constexpr Vector<T, Size3>(const Vector<U, 4>& _xyz)
			: x(static_cast<T>(_xyz.x)),
			y(static_cast<T>(_xyz.y)),
			z(static_cast<T>(_xyz.z)) {}


		template <typename U>
		constexpr Vector<T, Size3>& operator=(const Vector<U, 4>& _xyz)
		{
			x = static_cast<T>(_xyz.x);
			y = static_cast<T>(_xyz.y);
			z = static_cast<T>(_xyz.z);

			return *this;
		}

	// Public Methods
	public:
		constexpr SIZE_T Size() const
		{
			return Size3;
		}

		constexpr T* Data()
		{
			return &x;
		}

		constexpr const T* Data() const
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

		constexpr void Fill(T value)
		{
			x = value;
			y = value;
			z = value;
		}

		constexpr Vector& Apply(T(*func)(T))
		{
			x = func(x);
			y = func(y);
			z = func(z);
			return *this;
		}

		constexpr T SqrLength() const
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
			return length == 0 ? *this : *this /= static_cast<T>(length);
		}

		constexpr Vector GetNormalized() const
		{
			float length = Length();
			return length == 0 ? Vector(*this) : Vector(*this) /= static_cast<T>(length);
		}

	// Operators
	public:
		constexpr const T& operator[](SIZE_T idx) const
		{
			return *(&x + idx);
		}

		constexpr T& operator[](SIZE_T idx)
		{
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

		constexpr Vector& operator*=(const Vector& other)
		{
			x *= other.x;
			y *= other.y;
			z *= other.z;
			return *this;
		}

		constexpr Vector& operator/=(const Vector& other)
		{
			x /= other.x;
			y /= other.y;
			z /= other.z;
			return *this;
		}

		constexpr Vector& operator+=(T scalar)
		{
			x += scalar;
			y += scalar;
			z += scalar;
			return *this;
		}

		constexpr Vector& operator-=(T scalar)
		{
			x -= scalar;
			y -= scalar;
			z -= scalar;
			return *this;
		}

		constexpr Vector& operator*=(T scalar)
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}

		constexpr Vector& operator/=(T scalar)
		{
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

		constexpr Vector operator*(const Vector& other) const
		{
			return Vector(x * other.x, y * other.y, z * other.z);
		}

		constexpr Vector operator/(const Vector& other) const
		{
			return Vector(x / other.x, y / other.y, z / other.z);
		}

		constexpr Vector operator+(T scalar) const
		{
			return Vector(x + scalar, y + scalar, z + scalar);
		}

		constexpr Vector operator-(T scalar) const
		{
			return Vector(x - scalar, y - scalar, z - scalar);
		}

		constexpr Vector operator*(T scalar) const
		{
			return Vector(x * scalar, y * scalar, z * scalar);
		}

		constexpr Vector operator/(T scalar) const
		{
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
		static constexpr Vector up()
		{
			return Vector(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));
		}

		static constexpr Vector down()
		{
			return Vector(static_cast<T>(0), static_cast<T>(-1), static_cast<T>(0));
		}

		static constexpr Vector left()
		{
			return Vector(static_cast<T>(-1), static_cast<T>(0), static_cast<T>(0));
		}

		static constexpr Vector right()
		{
			return Vector(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
		}

		static constexpr Vector forward()
		{
			return Vector(static_cast<T>(0), static_cast<T>(0), static_cast<T>(-1));
		}

		static constexpr Vector back()
		{
			return Vector(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));
		}

	public:
		T x, y, z;
	};

#undef Size3
}
