#pragma once

#include "Vector.h"


namespace Athena
{
#define Size4 4

	template <typename Ty>
	class Vector<Ty, Size4>
	{
	public:
		using iterator = VectorIterator<Ty, Size4>;
		using const_iterator = VectorConstIterator<Ty, Size4>;

	// Constructors
	public:
		constexpr Vector() = default;

		constexpr Vector(Ty value)
			: x(value), 
			  y(value), 
			  z(value), 
			  w(value) {}


		constexpr Vector(Ty _x, Ty _y, Ty _z, Ty _w)
			: x(_x), 
			  y(_y), 
			  z(_z), 
			  w(_w) {}


		template<typename X, typename Y, typename Z, typename W>
		constexpr Vector(X _x, Y _y, Z _z, W _w)
			: x(static_cast<Ty>(_x)),
			  y(static_cast<Ty>(_y)),
			  z(static_cast<Ty>(_z)),
			  w(static_cast<Ty>(_w)) {}


		constexpr Vector(const Vector& other) = default;
		constexpr Vector& operator=(const Vector& other) = default;

		template <typename U>
		constexpr Vector<Ty, Size4>(const Vector<U, Size4>& other)
			: x(static_cast<Ty>(other.x)),
			  y(static_cast<Ty>(other.y)),
			  z(static_cast<Ty>(other.z)),
			  w(static_cast<Ty>(other.w)) {}


		template <typename U>
		constexpr Vector<Ty, Size4>& operator=(const Vector<U, Size4>& other)
		{
			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);
			z = static_cast<Ty>(other.z);
			w = static_cast<Ty>(other.w);

			return *this;
		}


		template <typename U>
		constexpr Vector<Ty, Size4>(const Vector<U, 2>& _xy)
			: x(static_cast<Ty>(_xy.x)),
			  y(static_cast<Ty>(_xy.y)),
			  z(static_cast<Ty>(1)),
			  w(static_cast<Ty>(1)) {}
			  

		template <typename U>
		constexpr Vector<Ty, Size4>& operator=(const Vector<U, 2>& _xy)
		{
			x = static_cast<Ty>(_xy.x);
			y = static_cast<Ty>(_xy.y);
			z = static_cast<Ty>(1);
			w = static_cast<Ty>(1);

			return *this;
		}


		template <typename X, typename Y, typename Z>
		constexpr Vector<Ty, Size4>(const Vector<X, 2>& _xy, Y _z, Z _w)
			: x(static_cast<Ty>(_xy.x)),
			  y(static_cast<Ty>(_xy.y)),
			  z(static_cast<Ty>(_z)),
		      w(static_cast<Ty>(_w)) {}


		template <typename X, typename Y, typename Z>
		constexpr Vector<Ty, Size4>(X _x, Y _y, const Vector<Z, 2>& _zw)
			: x(static_cast<Ty>(_x)),
			  y(static_cast<Ty>(_y)),
			  z(static_cast<Ty>(_zw.x)),
		      w(static_cast<Ty>(_zw.y)) {}


		template <typename X, typename Y, typename Z>
		constexpr Vector<Ty, Size4>(X _x, const Vector<Y, 2>& _yz, Z _w)
			: x(static_cast<Ty>(_x)),
			  y(static_cast<Ty>(_yz.x)),
			  z(static_cast<Ty>(_yz.y)),
			  w(static_cast<Ty>(_w)) {}


		template <typename U>
		constexpr Vector<Ty, Size4>(const Vector<U, 3>& other)
			: x(static_cast<Ty>(other.x)),
			  y(static_cast<Ty>(other.y)),
			  z(static_cast<Ty>(other.z)),
			  w(static_cast<Ty>(1)) {}


		template <typename U>
		constexpr Vector<Ty, Size4>& operator=(const Vector<U, 3>& other)
		{
			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);
			z = static_cast<Ty>(other.z);
			w = static_cast<Ty>(1);

			return *this;
		}

		template <typename X, typename Y>
		constexpr Vector<Ty, Size4>(const Vector<X, 3>& _xyz, Y _w)
			: x(static_cast<Ty>(_xyz.x)),
			  y(static_cast<Ty>(_xyz.y)),
			  z(static_cast<Ty>(_xyz.z)),
			  w(static_cast<Ty>(_w)) {}


		template <typename X, typename Y>
		constexpr Vector<Ty, Size4>(X _x, const Vector<Y, 3>& _yzw)
			: x(static_cast<Ty>(_x)),
			  y(static_cast<Ty>(_yzw.x)),
			  z(static_cast<Ty>(_yzw.y)),
		      w(static_cast<Ty>(_yzw.z)) {}

	// Public Methods
	public:
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

		constexpr void Fill(Ty value)
		{
			x = value;
			y = value;
			z = value;
			w = value;
		}

		constexpr Vector& Apply(Ty (*func)(Ty))
		{
			x = func(x);
			y = func(y);
			z = func(z);
			w = func(w);
			return *this;
		}

		constexpr Ty SqrLength() const 
		{
			return x * x + y * y + z * z + w * w;
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
			ATN_CORE_ASSERT(idx < Size4, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr Ty& operator[](size_t idx)
		{
			ATN_CORE_ASSERT(idx < Size4, "Vector subscript out of range");
			return *(&x + idx);
		}

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

#undef Size4
}
