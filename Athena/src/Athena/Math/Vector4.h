#pragma once

#include "Vector.h"


namespace Athena
{
	static constexpr size_t Size4 = 4;

	template <typename Ty>
	class Vector<Ty, 4>
	{
	public:
		using iterator = VectorIterator<Ty, Size4>;
		using const_iterator = VectorConstIterator<Ty, Size4>;

	public:
		constexpr Vector(Ty value = static_cast<Ty>(0))
			: x(value), y(value), z(value), t(value) {}

		constexpr Vector(Ty X, Ty Y, Ty Z, Ty T)
			: x(X), y(Y), z(Z), t(T) {}

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
			t = static_cast<Ty>(1);
		}

		template <typename U>
		constexpr Vector<Ty, Size4>(const Vector<U, Size4>& other)
		{
			static_assert(std::is_convertible<U, Ty>::value,
				"Vector initialization error: Vectors are not convertible");

			x = static_cast<Ty>(other.x);
			y = static_cast<Ty>(other.y);
			z = static_cast<Ty>(other.z);
			t = static_cast<Ty>(other.t);
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
			t = static_cast<Ty>(other.t);

			return *this;
		}

		constexpr void Fill(Ty value)
		{
			x = value;
			y = value;
			z = value;
			t = value;
		}

		constexpr Ty operator[](size_t idx) const
		{
			ATN_CORE_ASSERT(idx < Size4, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr Ty& operator[](size_t idx)
		{
			ATN_CORE_ASSERT(idx < Size4, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr size_t GetSize() const noexcept
		{
			return 4;
		}

		constexpr Ty* Data() noexcept
		{
			return &x;
		}

		constexpr Ty* Data() const noexcept
		{
			return &x;
		}

		constexpr iterator begin() noexcept
		{
			return iterator(&x, 0);
		}

		constexpr iterator end() noexcept
		{
			return iterator(&x, Size4);
		}

		constexpr const_iterator cbegin() const noexcept
		{
			return const_iterator(&x, 0);
		}

		constexpr const_iterator cend() const noexcept
		{
			return const_iterator(&x, Size4);
		}

		constexpr Ty GetSqrLength() const noexcept
		{
			return x * x + y * y + z * z + t * t;
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
			z += other.z;
			t += other.t;
			return *this;
		}

		constexpr Vector& operator-=(const Vector& other) noexcept
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			t -= other.t;
			return *this;
		}

		constexpr Vector& operator+=(Ty scalar) noexcept
		{
			x += scalar;
			y += scalar;
			z += scalar;
			t += scalar;
			return *this;
		}

		constexpr Vector& operator-=(Ty scalar) noexcept
		{
			x -= scalar;
			y -= scalar;
			z -= scalar;
			t -= scalar;
			return *this;
		}

		constexpr Vector& operator*=(Ty scalar) noexcept
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			t *= scalar;
			return *this;
		}

		constexpr Vector& operator/=(Ty scalar)
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector4 error: dividing by zero");
			x /= scalar;
			y /= scalar;
			z /= scalar;
			t /= scalar;
			return *this;
		}

		constexpr Vector operator+(const Vector& other) const noexcept
		{
			return Vector(x + other.x, y + other.y, z + other.z, t + other.t);
		}

		constexpr Vector operator-(const Vector& other) const noexcept
		{
			return Vector(x - other.x, y - other.y, z - other.z, t - other.t);
		}

		constexpr Vector operator+(Ty scalar) const noexcept
		{
			return Vector(x + scalar, y + scalar, z + scalar, t + scalar);
		}

		constexpr Vector operator-(Ty scalar) const noexcept
		{
			return Vector(x - scalar, y - scalar, z - scalar, t - scalar);
		}

		constexpr Vector operator*(Ty scalar) const noexcept
		{
			return Vector(x * scalar, y * scalar, z * scalar, t * scalar);
		}

		constexpr Vector operator/(Ty scalar) const
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector4 error: dividing by zero");
			return Vector(x / scalar, y / scalar, z / scalar, t / scalar);
		}

		constexpr Vector operator-() const noexcept
		{
			return Vector(-x, -y, -z, -t);
		}

		constexpr bool operator==(const Vector& other) const noexcept
		{
			return x == other.x && y == other.y && z == other.z && t == other.t;
		}

		constexpr bool operator!=(const Vector& other) const noexcept
		{
			return !(*this == other);
		}

	public:
		Ty x, y, z, t;
	};


	template <typename Ty>
	constexpr Ty Dot(const Vector<Ty, Size4>& first, const Vector<Ty, Size4>& second) noexcept
	{
		return first.x * second.x + first.y * second.y + first.z * second.z + first.t * second.t;
	}


	typedef Vector<float, Size4> Vector4;
	typedef Vector<int, Size4> Vector4i;
	typedef Vector<double, Size4> Vector4d;
}
