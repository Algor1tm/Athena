#pragma once


#include "Athena/Math/SIMD/Platform.h"


#ifdef ATN_SSE

#include "Athena/Math/Types/Vector.h"
#include "Athena/Math/Utils/Exponential.h"

namespace Athena
{
#define Size4 4

	// Forward declaration (VectorRelational_simd.h)
	inline float Dot(const Vector<float, 4>& left, const Vector<float, 4>& right);


	template <>
	class Vector <float, Size4>
	{
	public:
		using iterator = VectorIterator<float, Size4>;
		using const_iterator = VectorConstIterator<float, Size4>;

// -------------Constructors-------------------------------------
	public:
		inline Vector() = default;

		explicit inline Vector(float scalar)
			: _xmm(_mm_set_ps1(scalar)) {}
		

		constexpr Vector(__m128 data)
			: _xmm(data) {}


		template<typename X, typename Y, typename Z, typename W>
		inline Vector(X _x, Y _y, Z _z, W _w)
			: _xmm(_mm_set_ps(static_cast<float>(_w), 
				static_cast<float>(_z), 
				static_cast<float>(_y), 
				static_cast<float>(_x))) {}


		constexpr Vector(const Vector<float, Size4>& other)
			: _xmm(other._xmm) {}


		constexpr Vector& operator=(const Vector<float, Size4>& other)
		{
			if (other != *this)
				_xmm = other._xmm;

			return *this;
		}


		template <typename U>
		inline Vector<float, Size4>(const Vector<U, Size4>& other)
			: _xmm(_mm_set_ps(static_cast<float>(other.w),
				static_cast<float>(other.z),
				static_cast<float>(other.y),
				static_cast<float>(other.x))) {}


		template <typename U>
		inline Vector<float, Size4>& operator=(const Vector<U, Size4>& other)
		{
			_xmm = other._xmm;
			return *this;
		}


		template <typename U>
		inline Vector<float, Size4>(const Vector<U, 2>& other)
			: _xmm(_mm_set_ps(1.f,
				1.f,
				static_cast<float>(other.y),
				static_cast<float>(other.x))) {}		


		template <typename U>
		inline Vector<float, Size4>& operator=(const Vector<U, 2>& other)
		{
			_xmm = _mm_set_ps(1.f,
				1.f,
				static_cast<float>(other.y),
				static_cast<float>(other.x));

			return *this;
		}


		template <typename X, typename Y, typename Z>
		inline Vector<float, Size4>(const Vector<X, 2>& _xy, Y _z, Z _w)
			: _xmm(_mm_set_ps(static_cast<float>(_w),
				static_cast<float>(_z),
				static_cast<float>(_xy.y),
				static_cast<float>(_xy.x))) {}


		template <typename X, typename Y, typename Z>
		inline Vector<float, Size4>(X _x, Y _y, const Vector<Z, 2>& _zw)
			: _xmm(_mm_set_ps(static_cast<float>(_zw.y),
				static_cast<float>(_zw.x),
				static_cast<float>(_y),
				static_cast<float>(_x))) {}


		template <typename X, typename Y, typename Z>
		inline Vector<float, Size4>(X _x, const Vector<Y, 2>& _yz, Z _w)
			: _xmm(_mm_set_ps(static_cast<float>(_w),
				static_cast<float>(_yz.y),
				static_cast<float>(_yz.x),
				static_cast<float>(_x))) {}


		template <typename U>
		inline Vector<float, Size4>(const Vector<U, 3>& other)
		{
			_xmm = _mm_set_ps(1.f,
				static_cast<float>(other.z),
				static_cast<float>(other.y),
				static_cast<float>(other.x));
		}


		template <typename U>
		inline Vector<float, Size4>& operator=(const Vector<U, 3>& other)
		{
			_xmm = _mm_set_ps(1.f,
				static_cast<float>(other.z),
				static_cast<float>(other.y),
				static_cast<float>(other.x));

			return *this;
		}

		template <typename X, typename Y>
		inline Vector<float, Size4>(const Vector<X, 3>& _xyz, Y _w)
			: _xmm(_mm_set_ps(static_cast<float>(_w),
				static_cast<float>(_xyz.z),
				static_cast<float>(_xyz.y),
				static_cast<float>(_xyz.x))) {}


		template <typename X, typename Y>
		inline Vector<float, Size4>(X _x, const Vector<Y, 3>& _yzw)
			: _xmm(_mm_set_ps(static_cast<float>(_yzw.z),
				static_cast<float>(_yzw.y),
				static_cast<float>(_yzw.x),
				static_cast<float>(_x))) {}

// -------------Public Methods-------------------------------------
	public:
		constexpr SIZE_T Size() const
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

		inline void Fill(float value)
		{
			_xmm = _mm_set_ps1(value);
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
			return Sqrt(SqrLength());
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
	
// -------------Operators-------------------------------------
	public:
		constexpr const float& operator[](SIZE_T idx) const
		{
			ATN_CORE_ASSERT(idx < Size4, "Vector subscript out of range");
			return *(&x + idx);
		}

		constexpr float& operator[](SIZE_T idx)
		{
			ATN_CORE_ASSERT(idx < Size4, "Vector subscript out of range");
			return *(&x + idx);
		}

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

		inline Vector& operator*=(const Vector& other)
		{
			_xmm = _mm_mul_ps(_xmm, other._xmm);
			return *this;
		}

		inline Vector& operator/=(const Vector& other)
		{
			_xmm = _mm_div_ps(_xmm, other._xmm);
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

		constexpr Vector operator*(const Vector& other) const
		{
			return Vector(_mm_mul_ps(_xmm, other._xmm));
		}

		constexpr Vector operator/(const Vector& other) const
		{
			return Vector(_mm_div_ps(_xmm, other._xmm));
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
			return x != other.x || y != other.y || z != other.z || w != other.w;
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

#endif // ATN_SSE
