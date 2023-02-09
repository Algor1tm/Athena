#pragma once

#include "Matrix.h"
#include "Vector3.h"
#include "Vector4.h"

#include "Athena/Math/Common.h"
#include "Athena/Math/Trigonometric.h"


namespace Athena::Math
{
	template <typename T>
	class Quaternion;

	template <typename T>
	constexpr T Dot(const Quaternion<T>& left, const Quaternion<T>& right);


	template <typename T>
	class Quaternion
	{
// -------------Constructors-------------------------------------
	public:
		constexpr Quaternion() = default;

		template<typename W, typename X, typename Y, typename Z>
		constexpr Quaternion(W _w, X _x, Y _y, Z _z)
			: w(static_cast<T>(_w)), 
		      x(static_cast<T>(_x)), 
			  y(static_cast<T>(_y)), 
			  z(static_cast<T>(_z)) {}

		template <typename Q>
		constexpr Quaternion(const Vector<Q, 4>& vec4)
			: w(static_cast<T>(vec4.x)),
			  x(static_cast<T>(vec4.y)),
			  y(static_cast<T>(vec4.z)),
			  z(static_cast<T>(vec4.w)) {}


// -------------Public Methods-------------------------------------
	public:
		constexpr uint32 Size() const
		{
			return 4;
		}

		constexpr T* Data()
		{
			return &w;
		}

		constexpr const T* Data() const
		{
			return &w;
		}

		constexpr T SqrLength() const
		{
			return Math::Dot(*this, *this);
		}

		constexpr float Length() const
		{
			return Math::Sqrt(SqrLength());
		}

		constexpr Quaternion& Normalize()
		{
			float length = Length();
			return length == 0 ? *this : *this /= static_cast<T>(length);
		}

		constexpr Quaternion GetNormalized() const
		{
			float length = Length();
			return length == 0 ? Quaternion(*this) : Quaternion(*this) /= static_cast<T>(length);
		}
		
		constexpr Quaternion& Inverse()
		{
			T sqrLength = SqrLength();
			x = -x;
			y = -y;
			z = -z;
			return *this /= sqrLength;
		}

		constexpr Quaternion GetInversed() const
		{
			Quaternion conj = GetConjugated();
			T sqrLength = SqrLength();
			return conj / sqrLength;
		}

		constexpr Quaternion& Conjugate()
		{
			x = -x;
			y = -y;
			z = -z;
			return *this;
		}

		constexpr Quaternion GetConjugated() const
		{
			return Quaternion(w, -x, -y, -z);
		}

		inline Quaternion& Rotate(T radians, const Vector<T, 3>& axis)
		{
			Vector<T, 3> tmp = axis;

			T len = tmp.Length();
			if (Math::Abs(len - static_cast<T>(1.f)) > static_cast<T>(0.001f))
			{
				T oneOverLen = static_cast<T>(1) / len;
				tmp *= oneOverLen;
			}

			T sin = Math::Sin(radians * static_cast<T>(0.5));

			return *this *= Quaternion(Math::Cos(radians * static_cast<T>(0.5)), tmp.x * sin, tmp.y * sin, tmp.z * sin);
		}

		inline Quaternion GetRotated(T angle, const Vector<T, 3>& axis) const
		{
			return Quaternion(*this).Rotate(angle, axis);
		}

// -------------Operators-------------------------------------
	public:
		constexpr const T& operator[](uint32 idx) const
		{
			ATN_CORE_ASSERT(idx < 4, "Quaternion subscript out of range");
			return *(&w + idx);
		}

		constexpr T& operator[](uint32 idx)
		{
			ATN_CORE_ASSERT(idx < 4, "Quaternion subscript out of range");
			return *(&w + idx);
		}

		constexpr Quaternion& operator+=(const Quaternion& other)
		{
			w += other.w;
			x += other.x;
			y += other.y;
			z += other.z;
			return *this;
		}

		constexpr Quaternion& operator-=(const Quaternion& other)
		{
			w -= other.w;
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}

		constexpr Quaternion& operator*=(const Quaternion& other)
		{
			Quaternion<T> p(*this);

			w = p.w * other.w - p.x * other.x - p.y * other.y - p.z * other.z;
			x = p.w * other.x + p.x * other.w + p.y * other.z - p.z * other.y;
			y = p.w * other.y + p.y * other.w + p.z * other.x - p.x * other.z;
			z = p.w * other.z + p.z * other.w + p.x * other.y - p.y * other.x;
			return *this;
		}

		constexpr Quaternion& operator*=(T scalar)
		{
			w *= scalar;
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}

		constexpr Quaternion& operator/=(T scalar)
		{
			w /= scalar;
			x /= scalar;
			y /= scalar;
			z /= scalar;
			return *this;
		}

		constexpr Vector<T, 3> operator*(const Vector<T, 3>& vec3) const
		{
			Vector<T, 3> quatVector(x, y, z);
			Vector<T, 3> uv = Math::Cross(quatVector, vec3);
			Vector<T, 3> uuv = Math::Cross(quatVector, uv);

			return vec3 + ((uv * w) + uuv) * static_cast<T>(2);
		}

		constexpr Quaternion operator+(const Quaternion& other) const
		{
			return Quaternion(w + other.w, x + other.x, y + other.y, z + other.z);
		}

		constexpr Quaternion operator-(const Quaternion& other) const
		{
			return Quaternion(w - other.w, x - other.x, y - other.y, z - other.z);
		}

		constexpr Quaternion operator*(const Quaternion& other) const
		{
			return Quaternion(*this) *= other;
		}

		constexpr Quaternion operator*(T scalar) const
		{
			return Quaternion(w * scalar, x * scalar, y * scalar, z * scalar);
		}

		constexpr Quaternion operator/(T scalar) const
		{
			return Quaternion(w / scalar, x / scalar, y / scalar, z / scalar);
		}

		constexpr bool operator==(const Quaternion& other) const
		{
			return w == other.w && x == other.x && y == other.y && z == other.z;
		}

		constexpr bool operator!=(const Quaternion& other) const
		{
			return w != other.w || x != other.x || y != other.y || z != other.z;
		}

// -------------Static Methods-------------------------------------
	public:
		static constexpr Quaternion Identity()
		{
			return Quaternion(1, 0, 0, 0);
		}

	public:
		T w, x, y, z;
	};



// -------------Relative Functions-------------------------------------

	template <typename T>
	constexpr Quaternion<T> operator*(T scalar, const Quaternion<T>& quat)
	{
		return quat * scalar;
	}

	template <typename T>
	constexpr Quaternion<T> operator*(const Vector<T, 3> vec, const Quaternion<T>& quat)
	{
		return quat.GetInversed() * vec;
	}

	template <typename T>
	constexpr T Dot(const Quaternion<T>& left, const Quaternion<T>& right)
	{
		return left.w * right.w + left.x * right.x + left.y * right.y + left.z * right.z;
	}
}

namespace Athena
{
	template <typename T>
	inline String ToString(const Math::Quaternion<T>& quat)
	{
		std::stringstream stream;
		stream << "Quaternion(" << quat.w << ", " << quat.x << ", " << quat.y << ", " << quat.z << ")";

		return stream.str();
	}
}
