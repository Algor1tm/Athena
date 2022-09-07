#pragma once

#include "Vector.h"


namespace Athena::Math
{
#define Size2 2

	template <typename T>
	class Vector<T, Size2>
	{
	public:
		using iterator = VectorIterator<T, Size2>;
		using const_iterator = VectorConstIterator<T, Size2>;

// -------------Constructors-------------------------------------
	public:
		constexpr Vector() = default;

		explicit constexpr Vector(T scalar)
			: x(scalar), 
			  y(scalar) {}


		template<typename X, typename Y>
		constexpr Vector(X _x, Y _y)
			: x(static_cast<T>(_x)), 
			  y(static_cast<T>(_y)) {}


		template <typename U>
		constexpr Vector<T, Size2>(const Vector<U, Size2>& other)
			: x(static_cast<T>(other.x)),
		      y(static_cast<T>(other.y)) {}


		template <typename U>
		constexpr Vector<T, Size2>& operator=(const Vector<U, Size2>& other)
		{
			x = static_cast<T>(other.x);
			y = static_cast<T>(other.y);

			return *this;
		}


		template <typename U>
		constexpr Vector<T, Size2>(const Vector<U, 3>& _xy)
			: x(static_cast<T>(_xy.x)),
			  y(static_cast<T>(_xy.y)) {}


		template <typename U>
		constexpr Vector<T, Size2>& operator=(const Vector<U, 3>& _xy)
		{
			x = static_cast<T>(_xy.x);
			y = static_cast<T>(_xy.y);

			return *this;
		}


		template <typename U>
		constexpr Vector<T, Size2>(const Vector<U, 4>& _xy)
			: x(static_cast<T>(_xy.x)),
			  y(static_cast<T>(_xy.y)) {}


		template <typename U>
		constexpr Vector<T, Size2>& operator=(const Vector<U, 4>& _xy)
		{
			x = static_cast<T>(_xy.x);
			y = static_cast<T>(_xy.y);

			return *this;
		}

// -------------Public Methods-------------------------------------
	public:
		constexpr SIZE_T Size() const
		{
			return Size2;
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
			return iterator(&x, Size2);
		}

		constexpr const_iterator cbegin() const 
		{
			return const_iterator(&x, 0);
		}

		constexpr const_iterator cend() const 
		{
			return const_iterator(&x, Size2);
		}

		constexpr void Fill(T value)
		{
			x = value;
			y = value;
		}

		constexpr Vector& Apply(T (*func)(T))
		{
			x = func(x);
			y = func(y);
			return *this;
		}

		constexpr T SqrLength() const 
		{
			return Math::Dot(*this, *this);
		}

		constexpr float Length() const 
		{
			return Math::Sqrt(SqrLength());
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

// -------------Operators-------------------------------------
	public:
		constexpr const T& operator[](SIZE_T idx) const
		{
			ATN_CORE_ASSERT(idx < Size2, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr T& operator[](SIZE_T idx)
		{
			ATN_CORE_ASSERT(idx < Size2, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr Vector& operator+=(const Vector& other) 
		{
			x += other.x;
			y += other.y;
			return *this;
		}

		constexpr Vector& operator-=(const Vector& other) 
		{
			x -= other.x;
			y -= other.y;
			return *this;
		}

		constexpr Vector& operator*=(const Vector& other)
		{
			x *= other.x;
			y *= other.y;
			return *this;
		}

		constexpr Vector& operator/=(const Vector& other)
		{
			x /= other.x;
			y /= other.y;
			return *this;
		}

		constexpr Vector& operator+=(T scalar) 
		{
			x += scalar;
			y += scalar;
			return *this;
		}

		constexpr Vector& operator-=(T scalar) 
		{
			x -= scalar;
			y -= scalar;
			return *this;
		}

		constexpr Vector& operator*=(T scalar) 
		{
			x *= scalar;
			y *= scalar;
			return *this;
		}

		constexpr Vector& operator/=(T scalar)
		{
			x /= scalar;
			y /= scalar;
			return *this;
		}

		constexpr Vector operator+(const Vector& other) const 
		{
			return Vector(x + other.x, y + other.y);
		}

		constexpr Vector operator-(const Vector& other) const 
		{
			return Vector(x - other.x, y - other.y);
		}

		constexpr Vector operator*(const Vector& other) const
		{
			return Vector(x * other.x, y * other.y);
		}

		constexpr Vector operator/(const Vector& other) const
		{
			return Vector(x / other.x, y / other.y);
		}

		constexpr Vector operator+(T scalar) const 
		{
			return Vector(x + scalar, y + scalar);
		}

		constexpr Vector operator-(T scalar) const 
		{
			return Vector(x - scalar, y - scalar);
		}

		constexpr Vector operator*(T scalar) const 
		{
			return Vector(x * scalar, y * scalar);
		}

		constexpr Vector operator/(T scalar) const
		{
			return Vector(x / scalar, y / scalar);
		}

		constexpr Vector operator-() const 
		{
			return Vector(-x, -y);
		}

		constexpr bool operator==(const Vector& other) const 
		{
			return x == other.x && y == other.y;
		}

		constexpr bool operator!=(const Vector& other) const 
		{
			return x != other.x || y != other.y;
		}

	// Static Methods
	public:
		static  Vector up()
		{
			return Vector(static_cast<T>(0), static_cast<T>(1));
		}

		static constexpr Vector down()
		{
			return Vector(static_cast<T>(0), static_cast<T>(-1));
		}

		static constexpr Vector left()
		{
			return Vector(static_cast<T>(-1), static_cast<T>(0));
		}

		static constexpr Vector right()
		{
			return Vector(static_cast<T>(1), static_cast<T>(0));
		}

	public:
		T x, y;
	};

#undef Size2


// -------------Relative Functions-------------------------------------
	template <typename T>
	constexpr T Dot(const Vector<T, 2>& left, const Vector<T, 2>& right)
	{
		return left.x * right.x + left.y * right.y;
	}

	template <typename T>
	constexpr T Cross(const Vector<T, 2>& left, const Vector<T, 2>& right)
	{
		return left.x * right.y - left.y * right.x;
	}

	// @param normal: Expected to be normalized
	template <typename T>
	constexpr Vector<T, 2> Reflect(
		const Vector<T, 2>& direction, const Vector<T, 2>& normal)
	{
		return direction - 2 * normal * Math::Dot(direction, normal);
	}
}
