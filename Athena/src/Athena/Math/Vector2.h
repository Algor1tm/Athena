#pragma once

#include "Vector.h"


namespace Athena
{
	static constexpr size_t Size2 = 2;

	template <typename Ty>
	class Vector<Ty, Size2>
	{
	public:
		using iterator = VectorIterator<Ty, Size2>;
		using const_iterator = VectorConstIterator<Ty, Size2>;

	public:
		constexpr Vector() = default;

		constexpr Vector(Ty value)
			: x(value), y(value) {}

		constexpr Vector(Ty X, Ty Y)
			: x(X), y(Y) {}

		constexpr Vector(const Vector& other) = default;

		template <typename U>
		constexpr Vector<Ty, Size2>(const Vector<U, Size2>& other)
		{
			static_assert(std::is_convertible<U, Ty>::value,
				"Vector initialization error: Vectors are not convertible");

			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);
		}

		constexpr Vector& operator=(const Vector& other) = default;

		template <typename U>
		constexpr Vector<Ty, Size2>& operator=(const Vector<U, Size2>& other)
		{
			static_assert(std::is_convertible<U, Ty>::value,
				"Vector assignment error: Vectors are not convertible");

			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);

			return *this;
		}

		constexpr void Fill(Ty value)
		{
			x = value;
			y = value;
		}

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

		constexpr size_t GetSize() const 
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

		constexpr Vector& Apply(Ty (*func)(Ty))
		{
			x = func(x);
			y = func(y);
			return *this;
		}

		constexpr Ty GetSqrLength() const 
		{
			return x * x + y * y;
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

	public:
		static const Vector<Ty, Size2> up;
		static const Vector<Ty, Size2> down;
		static const Vector<Ty, Size2> left;
		static const Vector<Ty, Size2> right;

	public:
		Ty x, y;
	};

	template <typename Ty>
	constexpr Ty Dot(const Vector<Ty, Size2>& Left, const Vector<Ty, Size2>& Right)  
	{
		return Left.x * Right.x + Left.y * Right.y;
	}

	template <typename Ty>
	constexpr Ty Cross(const Vector<Ty, Size2>& Left, const Vector<Ty, Size2>& Right) 
	{
		return Left.x * Right.y - Left.y * Right.x;
	}

	/// <param name="Normal">: Must be normalized</param>
	template <typename Ty>
	constexpr Vector<Ty, Size2> Reflect(
		const Vector<Ty, Size2>& Direction, const Vector<Ty, Size2>& Normal) 
	{
		return Direction - 2 * Normal * Dot(Direction, Normal);
	}


	template<typename Ty>
	const Vector<Ty, Size2> Vector<Ty, Size2>::up = 
		Vector<Ty, Size2>(static_cast<Ty>(0), static_cast<Ty>(-1));

	template<typename Ty>
	const Vector<Ty, Size2> Vector<Ty, Size2>::down = 
		Vector<Ty, Size2>(static_cast<Ty>(0), static_cast<Ty>(1));

	template<typename Ty>
	const Vector<Ty, Size2> Vector<Ty, Size2>::left = 
		Vector<Ty, Size2>(static_cast<Ty>(-1), static_cast<Ty>(0));

	template<typename Ty>
	const Vector<Ty, Size2> Vector<Ty, Size2>::right = 
		Vector<Ty, Size2>(static_cast<Ty>(1), static_cast<Ty>(0));


	typedef Vector<float, Size2> Vector2;
	typedef Vector<unsigned int, Size2> Vector2u;
	typedef Vector<int, Size2> Vector2i;
	typedef Vector<double, Size2> Vector2d;
}
