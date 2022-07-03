#pragma once

#include "Athena/Math/Source/Vector.h"


namespace Athena
{
#define Size4 4

	float Dot(const Vector<float, 4>& Left, const Vector<float, 4>& Right);

	template <>
	class Vector <float, Size4>
	{
	public:
		using iterator = VectorIterator<float, Size4>;
		using const_iterator = VectorConstIterator<float, Size4>;

	public:
		Vector() = default;

		inline Vector(float value)
			: _xmm(_mm_set_ps1(value)) {}

		inline Vector(float X, float Y, float Z, float W)
			: _xmm(_mm_set_ps(W, Z, Y, X)) {}

		constexpr Vector(__m128 data)
			: _xmm(data) {}

		constexpr Vector(const Vector<float, Size4>& other)
			: _xmm(other._xmm) {}

		constexpr Vector& operator=(const Vector<float, Size4>& other)
		{
			if (other != *this)
				_xmm = other._xmm;
			return *this;
		}

		template <typename U>
		constexpr Vector(const Vector<U, Size4>& other)
		{
			static_assert(std::is_convertible<U, float>::value,
				"Vector initialization error: Vectors are not convertible");

			_xmm = _mm_set_ps(static_cast<float>(other.w),
				static_cast<float>(other.z),
				static_cast<float>(other.y),
				static_cast<float>(other.x));
		}

		template <typename U>
		constexpr Vector<float, Size4>& operator=(const Vector<U, Size4>& other)
		{
			static_assert(std::is_convertible<U, float>::value,
				"Vector assignment error: Vectors are not convertible");

			_xmm = other._xmm;
			return *this;
		}

		template <typename U>
		constexpr Vector(const Vector<U, 3>& other)
		{
			static_assert(std::is_convertible<U, float>::value,
				"Vector initialization error: Vectors are not convertible");

			_xmm = _mm_set_ps(1.f,
				static_cast<float>(other.z),
				static_cast<float>(other.y),
				static_cast<float>(other.x));
		}

		template <typename U>
		constexpr Vector& operator=(const Vector<U, 3>& other)
		{
			static_assert(std::is_convertible<U, float>::value,
				"Vector initialization error: Vectors are not convertible");

			_xmm = _mm_set_ps(1.f,
				static_cast<float>(other.z),
				static_cast<float>(other.y),
				static_cast<float>(other.x));

			return *this;
		}

		template <typename U>
		constexpr Vector(const Vector<U, 2>& other)
		{
			static_assert(std::is_convertible<U, float>::value,
				"Vector initialization error: Vectors are not convertible");

			_xmm = _mm_set_ps(1.f,
				1.f,
				static_cast<float>(other.y),
				static_cast<float>(other.x));
		}

		template <typename U>
		constexpr Vector& operator=(const Vector<U, 2>& other)
		{
			static_assert(std::is_convertible<U, float>::value,
				"Vector initialization error: Vectors are not convertible");

			_xmm = _mm_set_ps(1.f,
				1.f,
				static_cast<float>(other.y),
				static_cast<float>(other.x));

			return *this;
		}

		inline void Fill(float value)
		{
			_xmm = _mm_set_ps1(value);
		}

		constexpr const float& operator[](size_t idx) const
		{
			ATN_CORE_ASSERT(idx < Size4, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr float& operator[](size_t idx)
		{
			ATN_CORE_ASSERT(idx < Size4, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr size_t Size() const
		{
			return Size4;
		}

		constexpr float* Data()
		{
			return &x;
		}

		constexpr const float* Data() const
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

		inline Vector& Apply(float(*func)(float))
		{
			_xmm = _mm_set_ps(func(w), func(z), func(y), func(x));
			return *this;
		}

		inline float SqrLength() const
		{
			return Dot(*this, *this);
		}

		inline float Length() const
		{
			return std::sqrt(SqrLength());
		}

		inline Vector& Normalize()
		{
			float length = Length();
			return length == 0 ? *this : *this /= length;
		}

		inline Vector GetNormalized() const
		{
			float length = Length();
			return length == 0 ? Vector(*this) : *this / length;
		}

	public:
		inline Vector& operator+=(const Vector& other)
		{
			_xmm = _mm_add_ps(_xmm, other._xmm);
			return *this;
		}

		inline Vector& operator-=(const Vector& other)
		{
			_xmm = _mm_sub_ps(_xmm, other._xmm);
			return *this;
		}

		inline Vector& operator+=(float scalar)
		{
			_xmm = _mm_add_ps(_xmm, _mm_set_ps1(scalar));
			return *this;
		}

		inline Vector& operator-=(float scalar)
		{
			_xmm = _mm_sub_ps(_xmm, _mm_set_ps1(scalar));
			return *this;
		}

		inline Vector& operator*=(float scalar)
		{
			_xmm = _mm_mul_ps(_xmm, _mm_set_ps1(scalar));
			return *this;
		}

		inline Vector& operator/=(float scalar)
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector operation error: dividing by zero");
			_xmm = _mm_div_ps(_xmm, _mm_set_ps1(scalar));
			return *this;
		}

		constexpr Vector operator+(const Vector& other) const
		{
			return Vector(_mm_add_ps(_xmm, other._xmm));
		}

		constexpr Vector operator-(const Vector& other) const
		{
			return Vector(_mm_sub_ps(_xmm, other._xmm));
		}

		constexpr Vector operator+(float scalar) const
		{
			return Vector(_mm_add_ps(_xmm, _mm_set_ps1(scalar)));
		}

		constexpr Vector operator-(float scalar) const
		{
			return Vector(_mm_sub_ps(_xmm, _mm_set_ps1(scalar)));
		}

		constexpr Vector operator*(float scalar) const
		{
			return Vector(_mm_mul_ps(_xmm, _mm_set_ps1(scalar)));
		}

		constexpr Vector operator/(float scalar) const
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector operation error: dividing by zero");
			return Vector(_mm_div_ps(_xmm, _mm_set_ps1(scalar)));
		}

		constexpr Vector operator-() const
		{
			return Vector(_mm_set_ps(-w, -z, -y, -x));
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
		union
		{
			__m128 _xmm;
			struct
			{
				float x, y, z, w;
			};
		};
	};

#undef Size4
}
