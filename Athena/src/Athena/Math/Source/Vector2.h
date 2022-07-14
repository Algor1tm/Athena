#pragma once

#include "Vector.h"


namespace Athena
{
#define Size2 2

	template <typename Ty>
	class Vector<Ty, Size2>
	{
	public:
		using iterator = VectorIterator<Ty, Size2>;
		using const_iterator = VectorConstIterator<Ty, Size2>;

	// Constructors
	public:
		constexpr Vector() = default;

		constexpr Vector(Ty value)
			: x(value), 
			  y(value) {}

		constexpr Vector(Ty _x, Ty _y)
			: x(_x), 
			  y(_y) {}


		template<typename X, typename Y>
		constexpr Vector(X _x, Y _y)
			: x(static_cast<Ty>(_x)), 
			  y(static_cast<Ty>(_y)) {}


		constexpr Vector(const Vector& other) = default;
		constexpr Vector& operator=(const Vector& other) = default;


		template <typename U>
		constexpr Vector<Ty, Size2>(const Vector<U, Size2>& other)
			: x(static_cast<Ty>(other.x)),
		      y(static_cast<Ty>(other.y)) {}


		template <typename U>
		constexpr Vector<Ty, Size2>& operator=(const Vector<U, Size2>& other)
		{
			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);

			return *this;
		}


		template <typename U>
		constexpr Vector<Ty, Size2>(const Vector<U, 3>& _xy)
			: x(static_cast<Ty>(_xy.x)),
			  y(static_cast<Ty>(_xy.y)) {}


		template <typename U>
		constexpr Vector<Ty, Size2>& operator=(const Vector<U, 3>& _xy)
		{
			x = static_cast<Ty>(_xy.x);
			y = static_cast<Ty>(_xy.y);

			return *this;
		}


		template <typename U>
		constexpr Vector<Ty, Size2>(const Vector<U, 4>& _xy)
			: x(static_cast<Ty>(_xy.x)),
			  y(static_cast<Ty>(_xy.y)) {}


		template <typename U>
		constexpr Vector<Ty, Size2>& operator=(const Vector<U, 4>& _xy)
		{
			x = static_cast<Ty>(_xy.x);
			y = static_cast<Ty>(_xy.y);

			return *this;
		}

	// Public Methods
	public:
		constexpr size_t Size() const
		{
			return Size2;
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

		constexpr void Fill(Ty value)
		{
			x = value;
			y = value;
		}

		constexpr Vector& Apply(Ty (*func)(Ty))
		{
			x = func(x);
			y = func(y);
			return *this;
		}

		constexpr Ty SqrLength() const 
		{
			return x * x + y * y;
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
			ATN_CORE_ASSERT(idx < Size2, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr Ty& operator[](size_t idx)
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

		constexpr Vector& operator+=(Ty scalar) 
		{
			x += scalar;
			y += scalar;
			return *this;
		}

		constexpr Vector& operator-=(Ty scalar) 
		{
			x -= scalar;
			y -= scalar;
			return *this;
		}

		constexpr Vector& operator*=(Ty scalar) 
		{
			x *= scalar;
			y *= scalar;
			return *this;
		}

		constexpr Vector& operator/=(Ty scalar)
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector operation error: dividing by zero");
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

		constexpr Vector operator+(Ty scalar) const 
		{
			return Vector(x + scalar, y + scalar);
		}

		constexpr Vector operator-(Ty scalar) const 
		{
			return Vector(x - scalar, y - scalar);
		}

		constexpr Vector operator*(Ty scalar) const 
		{
			return Vector(x * scalar, y * scalar);
		}

		constexpr Vector operator/(Ty scalar) const
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector operation error: dividing by zero");
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
			return !(*this == other);
		}

	// Static Methods
	public:
		static Vector up()
		{
			return Vector(static_cast<Ty>(0), static_cast<Ty>(1));
		}

		static Vector down()
		{
			return Vector(static_cast<Ty>(0), static_cast<Ty>(-1));
		}

		static Vector left()
		{
			return Vector(static_cast<Ty>(-1), static_cast<Ty>(0));
		}

		static Vector right()
		{
			return Vector(static_cast<Ty>(1), static_cast<Ty>(0));
		}

	public:
		Ty x, y;
	};

#undef Size2
}
