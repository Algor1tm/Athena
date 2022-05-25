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
		constexpr Vector(Ty value = static_cast<Ty>(0))
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

		constexpr Ty operator[](size_t idx) const
		{
			ATN_CORE_ASSERT(idx < Size2, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr Ty& operator[](size_t idx)
		{
			ATN_CORE_ASSERT(idx < Size2, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr size_t GetSize() const noexcept
		{
			return Size2;
		}

		constexpr Ty* Data() noexcept
		{
			return &x;
		}

		constexpr iterator begin() noexcept
		{
			return iterator(&x, 0);
		}

		constexpr iterator end() noexcept
		{
			return iterator(&x, Size2);
		}

		constexpr const_iterator cbegin() const noexcept
		{
			return const_iterator(&x, 0);
		}

		constexpr const_iterator cend() const noexcept
		{
			return const_iterator(&x, Size2);
		}

		constexpr Ty* Data() const noexcept
		{
			return &x;
		}

		constexpr Ty GetSqrLength() const noexcept
		{
			return x * x + y * y;
		}

		constexpr float GetLength() const noexcept
		{
			return std::sqrt(static_cast<float>(GetSqrLength()));
		}

		constexpr Vector& Normalize()
		{
			float length = GetLength();
			return length == 0 ? *this : *this /= static_cast<Ty>(length);
		}

	public:
		constexpr Vector& operator+=(const Vector& other) noexcept
		{
			x += other.x;
			y += other.y;
			return *this;
		}

		constexpr Vector& operator-=(const Vector& other) noexcept
		{
			x -= other.x;
			y -= other.y;
			return *this;
		}

		constexpr Vector& operator+=(Ty scalar) noexcept
		{
			x += scalar;
			y += scalar;
			return *this;
		}

		constexpr Vector& operator-=(Ty scalar) noexcept
		{
			x -= scalar;
			y -= scalar;
			return *this;
		}

		constexpr Vector& operator*=(Ty scalar) noexcept
		{
			x *= scalar;
			y *= scalar;
			return *this;
		}

		constexpr Vector& operator/=(Ty scalar)
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector2 error: dividing by zero");
			x /= scalar;
			y /= scalar;
			return *this;
		}

		constexpr Vector operator+(const Vector& other) const noexcept
		{
			return Vector(x + other.x, y + other.y);
		}

		constexpr Vector operator-(const Vector& other) const noexcept
		{
			return Vector(x - other.x, y - other.y);
		}

		constexpr Vector operator+(Ty scalar) const noexcept
		{
			return Vector(x + scalar, y + scalar);
		}

		constexpr Vector operator-(Ty scalar) const noexcept
		{
			return Vector(x - scalar, y - scalar);
		}

		constexpr Vector operator*(Ty scalar) const noexcept
		{
			return Vector(x * scalar, y * scalar);
		}

		constexpr Vector operator/(Ty scalar) const
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector2 error: dividing by zero");
			return Vector(x / scalar, y / scalar);
		}

		constexpr Vector operator-() const noexcept
		{
			return Vector(-x, -y);
		}

		constexpr bool operator==(const Vector& other) const noexcept
		{
			return x == other.x && y == other.y;
		}

		constexpr bool operator!=(const Vector& other) const noexcept
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
	constexpr Ty Dot(const Vector<Ty, Size2>& first, const Vector<Ty, 2>& second)  noexcept
	{
		return first.x * second.x + first.y * second.y;
	}

	template <typename Ty>
	constexpr Ty Cross(const Vector<Ty, Size2>& first, const Vector<Ty, 2>& second) noexcept
	{
		return first.x * second.y - first.y * second.x;
	}

	template <typename Ty>
	constexpr Vector<Ty, 2> Reflect(const Vector<Ty, Size2>& From, const Vector<Ty, 2>& To) noexcept
	{
		Vector<Ty, 2> out = To;
		Ty projection = Dot(From, To) / From.GetSqrLength();
		out -= 2 * projection * From;
		return out;
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
	typedef Vector<int, Size2> Vector2i;
	typedef Vector<double, Size2> Vector2d;
}
