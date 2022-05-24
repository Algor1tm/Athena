#pragma once

#include "Vector.h"


namespace Athena
{
	static constexpr size_t Size3 = 3;

	template <typename Ty>
	class Vector<Ty, Size3>
	{
	public:
		using iterator = VectorIterator<Ty, Size3>;
		using const_iterator = VectorConstIterator<Ty, Size3>;

	public:
		constexpr Vector(Ty value = static_cast<Ty>(0))
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

		Ty operator[](size_t idx) const
		{
			ATN_CORE_ASSERT(idx < Size3, "Vector subscript out of range");
			return *(&x + idx);
		}

		Ty& operator[](size_t idx)
		{
			ATN_CORE_ASSERT(idx < Size3, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr size_t GetSize() const noexcept
		{
			return 3;
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
			return iterator(&x, Size3);
		}

		constexpr const_iterator cbegin() const noexcept
		{
			return const_iterator(&x, 0);
		}

		constexpr const_iterator cend() const noexcept
		{
			return const_iterator(&x, Size3);
		}

		constexpr Ty GetSqrLength(const Vector& vec) const noexcept
		{
			return x * x + y * y + z * z;
		}

		constexpr float GetLength(const Vector& vec) const noexcept
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
			return *this;
		}

		constexpr Vector& operator-=(const Vector& other) noexcept
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}

		constexpr Vector& operator+=(Ty scalar) noexcept
		{
			x += scalar;
			y += scalar;
			z += scalar;
			return *this;
		}

		constexpr Vector& operator-=(Ty scalar) noexcept
		{
			x -= scalar;
			y -= scalar;
			z -= scalar;
			return *this;
		}

		constexpr Vector& operator*=(Ty scalar) noexcept
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}

		constexpr Vector& operator/=(Ty scalar)
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector3 error: dividing by zero");
			x /= scalar;
			y /= scalar;
			z /= scalar;
			return *this;
		}

		constexpr Vector operator+(const Vector& other) const noexcept
		{
			return Vector(x + other.x, y + other.y, z + other.z);
		}

		constexpr Vector operator-(const Vector& other) const noexcept
		{
			return Vector(x - other.x, y - other.y, z - other.z);
		}

		constexpr Vector operator+(Ty scalar) const noexcept
		{
			return Vector(x + scalar, y + scalar, z + scalar);
		}

		constexpr Vector operator-(Ty scalar) const noexcept
		{
			return Vector(x - scalar, y - scalar, z - scalar);
		}

		constexpr Vector operator*(Ty scalar) const noexcept
		{
			return Vector(x * scalar, y * scalar, z * scalar);
		}

		constexpr Vector operator/(Ty scalar) const
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector3 error: dividing by zero");
			return Vector(x / scalar, y / scalar, z / scalar);
		}

		constexpr Vector operator-() const noexcept
		{
			return Vector(-x, -y, -z);
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

	template <typename Ty>
	constexpr Ty Dot(const Vector<Ty, Size3>& first, const Vector<Ty, Size3>& second)  noexcept
	{
		return first.x * second.x + first.y * second.y + first.z * second.z;
	}

	template <typename Ty>
	constexpr Vector<Ty, Size3> Cross(const Vector<Ty, 3>& first, const Vector<Ty, Size3>& second) noexcept
	{
		Vector3 out;
		out.x = first.y * second.z - first.z * second.y;
		out.y = -first.x * second.z + first.z * second.x;
		out.z = first.x * second.y - first.y * second.x;
		return out;
	}

	template <typename Ty>
	constexpr Vector<Ty, Size3> Reflect(const Vector<Ty, Size3>& From, const Vector<Ty, Size3>& To) noexcept
	{
		Vector<Ty, Size3> out = To;
		Ty projection = Dot(From, To) / From.GetSqrLength();
		out -= 2 * projection * From;
		return out;
	}


	template<typename Ty>
	const Vector<Ty, Size3> Vector<Ty, Size3>::up =
		Vector<Ty, 3>(static_cast<Ty>(0), static_cast<Ty>(-1), static_cast<Ty>(0));

	template<typename Ty>
	const Vector<Ty, Size3> Vector<Ty, Size3>::down =
		Vector<Ty, 3>(static_cast<Ty>(0), static_cast<Ty>(1), static_cast<Ty>(0));

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


	typedef Vector<float, Size3> Vector3;
	typedef Vector<int, Size3> Vector3i;
	typedef Vector<double, Size3> Vector3d;
}
