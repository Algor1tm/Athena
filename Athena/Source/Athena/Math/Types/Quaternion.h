#pragma once

#include "Matrix.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Athena/Math/Types/VectorRelational.h" 
#include "Athena/Math/VectorCommon.h"


namespace Athena::Math
{
	template <typename T>
	class Quaternion;

	// Forward declaration (QuaternionRelational.h)
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

		inline Quaternion(const Vector<T, 3>& eulerAngles)
		{
			Vector<T, 3> c = Math::Cos(eulerAngles * T(0.5));
			Vector<T, 3> s = Math::Sin(eulerAngles * T(0.5));

			w = c.x * c.y * c.z + s.x * s.y * s.z;
			x = s.x * c.y * c.z - c.x * s.y * s.z;
			y = c.x * s.y * c.z + s.x * c.y * s.z;
			z = c.x * c.y * s.z - s.x * s.y * c.z;
		}

		inline Quaternion(float radians, const Vector<T, 3>& axis)
		{
			T s = Math::Sin(radians * static_cast<T>(0.5));
			Vector<T, 3> v = axis * s;

			w = Math::Cos(a * static_cast<T>(0.5));
			x = v.x;
			y = v.y;
			z = v.z;
		}

		explicit constexpr Quaternion(const Matrix<T, 3, 3>& m)
		{
			T fourXSquaredMinus1 = m[0][0] - m[1][1] - m[2][2];
			T fourYSquaredMinus1 = m[1][1] - m[0][0] - m[2][2];
			T fourZSquaredMinus1 = m[2][2] - m[0][0] - m[1][1];
			T fourWSquaredMinus1 = m[0][0] + m[1][1] + m[2][2];

			int biggestIndex = 0;
			T fourBiggestSquaredMinus1 = fourWSquaredMinus1;
			if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
			{
				fourBiggestSquaredMinus1 = fourXSquaredMinus1;
				biggestIndex = 1;
			}
			if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
			{
				fourBiggestSquaredMinus1 = fourYSquaredMinus1;
				biggestIndex = 2;
			}
			if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
			{
				fourBiggestSquaredMinus1 = fourZSquaredMinus1;
				biggestIndex = 3;
			}

			T biggestVal = sqrt(fourBiggestSquaredMinus1 + static_cast<T>(1)) * static_cast<T>(0.5);
			T mult = static_cast<T>(0.25) / biggestVal;

			switch (biggestIndex)
			{
			case 0:
				w = biggestVal; 
				x = (m[1][2] - m[2][1]) * mult; 
				y = (m[2][0] - m[0][2]) * mult; 
				z = (m[0][1] - m[1][0]) * mult; 
				break;
			case 1:
				w = (m[1][2] - m[2][1]) * mult;
				x = biggestVal;
				y = (m[0][1] + m[1][0]) * mult;
				z = (m[2][0] + m[0][2]) * mult;
				break;
			case 2:
				w = (m[2][0] - m[0][2]) * mult;
				x = (m[0][1] + m[1][0]) * mult;
				y = biggestVal;
				z = (m[1][2] + m[2][1]) * mult;
				break;
			case 3:
				w = (m[0][1] - m[1][0]) * mult;
				x = (m[2][0] + m[0][2]) * mult;
				y = (m[1][2] + m[2][1]) * mult;
				z = biggestVal;
				break;
			default:
				ATN_CORE_ASSERT(false, "");
			}
		}

		explicit constexpr Quaternion(const Matrix<T, 4, 4>& mat4)
		{
			Matrix<T, 3, 3> mat3;
			mat3[0] = Vector<T, 3>(mat4[0]);
			mat3[1] = Vector<T, 3>(mat4[1]);
			mat3[2] = Vector<T, 3>(mat4[2]);

			return Quaternion(mat3);
		}

// -------------Type Casts-------------------------------------
	public:
		explicit constexpr operator Matrix<float, 3, 3>() const
		{
			Matrix<float, 3, 3> Result(T(1));
			T qxx(x * x);
			T qyy(y * y);
			T qzz(z * z);
			T qxz(x * z);
			T qxy(x * y);
			T qyz(y * z);
			T qwx(w * x);
			T qwy(w * y);
			T qwz(w * z);

			Result[0][0] = T(1) - T(2) * (qyy + qzz);
			Result[0][1] = T(2) * (qxy + qwz);
			Result[0][2] = T(2) * (qxz - qwy);

			Result[1][0] = T(2) * (qxy - qwz);
			Result[1][1] = T(1) - T(2) * (qxx + qzz);
			Result[1][2] = T(2) * (qyz + qwx);

			Result[2][0] = T(2) * (qxz + qwy);
			Result[2][1] = T(2) * (qyz - qwx);
			Result[2][2] = T(1) - T(2) * (qxx + qyy);
			return Result;
		}

		explicit constexpr operator Matrix<float, 4, 4>() const
		{
			Matrix<T, 3, 3> mat3(*this);
			Matrix<T, 4, 4> out;
			out[0] = Vector<T, 4>(mat3[0], 0);
			out[1] = Vector<T, 4>(mat3[1], 0);
			out[2] = Vector<T, 4>(mat3[2], 0);
			out[3] = Vector<T, 4>(0, 0, 0, 1);

			return out;
		}

// -------------Public Methods-------------------------------------
	public:
		constexpr SIZE_T Size() const
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
			return Dot(*this, *this);
		}

		constexpr float Length() const
		{
			return Sqrt(SqrLength());
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
		constexpr const T& operator[](SIZE_T idx) const
		{
			ATN_CORE_ASSERT(idx < 4, "Quaternion subscript out of range");
			return *(&w + idx);
		}

		constexpr T& operator[](SIZE_T idx)
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
			Quaternion<T> const p(*this);

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
			Vector<T, 3> uv = Cross(quatVector, vec3);
			Vector<T, 3> uuv = Cross(quatVector, uv);

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
			return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar);
		}

		constexpr Quaternion operator/(T scalar) const
		{
			return Quaternion(x / scalar, y / scalar, z / scalar, w / scalar);
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
}
