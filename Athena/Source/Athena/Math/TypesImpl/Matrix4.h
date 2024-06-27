#pragma once

#include "Athena/Math/TypesImpl/Matrix.h"
#include "Athena/Math/TypesImpl/Vector4.h"

#include "Athena/Math/Trigonometric.h"


namespace Athena::Math
{
#define Size4 4

	template<typename T>
	class Matrix<T, Size4, Size4>
	{
	public:
		using RowType = Vector<T, Size4>;
		using ColumnType = Vector<T, Size4>;

	public:
		using iterator = VectorIterator<Vector<T, Size4>, Size4>;
		using const_iterator = VectorConstIterator<Vector<T, Size4>, Size4>;

// -------------Constructors-------------------------------------
	public:
		constexpr Matrix() = default;

		explicit constexpr Matrix(T scalar)
		{
			Fill(scalar);
		}

		template<typename X0, typename X1, typename X2, typename X3,
		typename Y0, typename Y1, typename Y2, typename Y3,
		typename Z0, typename Z1, typename Z2, typename Z3,
		typename W0, typename W1, typename W2, typename W3>

		constexpr Matrix(X0 x0, X1 x1, X2 x2, X3 x3,
			Y0 y0, Y1 y1, Y2 y2, Y3 y3,
			Z0 z0, Z1 z1, Z2 z2, Z3 z3,
			W0 w0, W1 w1, W2 w2, W3 w3)
		{
			m_Array[0] = RowType(x0, x1, x2, x3);
			m_Array[1] = RowType(y0, y1, y2, y3);
			m_Array[2] = RowType(z0, z1, z2, z3);
			m_Array[3] = RowType(w0, w1, w2, w3);
		}

		constexpr Matrix(const std::initializer_list<RowType>& values)
		{
			ATN_CORE_ASSERT(values.size() == Size4, "Invalid initializer list");
			uint32 idx = 0;
			for (auto& row : values)
			{
				m_Array[idx] = row;
				++idx;
			}
		}

		template <typename U>
		constexpr Matrix(const Matrix<U, Size4, Size4>& other)
		{
			for (uint32 i = 0; i < Size4; ++i)
				m_Array[i] = static_cast<T>(other[i]);
		}

		template <typename U>
		constexpr Matrix(const Matrix<U, 3, 3>& other)
		{
			m_Array[0] = Vector<T, 4>(other[0], 0);
			m_Array[1] = Vector<T, 4>(other[1], 0);
			m_Array[2] = Vector<T, 4>(other[2], 0);
			m_Array[3] = Vector<T, 4>(0, 0, 0, 1);
		}

		template <typename U>
		constexpr Matrix<T, Size4, Size4>& operator=(const Matrix<U, Size4, Size4>& other)
		{
			for (uint32 i = 0; i < Size4; ++i)
				m_Array[i] = static_cast<T>(other[i]);
		}

// -------------Public Methods-------------------------------------
	public:
		constexpr T* Data()
		{
			return &(m_Array[0][0]);
		}

		constexpr const T* Data() const
		{
			return &(m_Array[0][0]);
		}

		constexpr ColumnType GetColumn(uint32 idx) const
		{
			ATN_CORE_ASSERT(idx < 4, "Matrix subscript out of range");
			ColumnType out(m_Array[0][idx], m_Array[1][idx], m_Array[2][idx], m_Array[3][idx]);

			return out;
		}

		constexpr uint32 SizeOfRow() const
		{
			return Size4;
		}

		constexpr uint32 SizeOfColumn() const
		{
			return Size4;
		}

		constexpr iterator begin()
		{
			return iterator(m_Array, 0);
		}

		constexpr iterator end()
		{
			return iterator(m_Array, 4);
		}

		constexpr const_iterator cbegin() const
		{
			return const_iterator(m_Array, 0);
		}

		constexpr const_iterator cend() const
		{
			return const_iterator(m_Array, 4);
		}

		constexpr Matrix& Fill(T value)
		{
			m_Array[0].Fill(value);
			m_Array[1].Fill(value);
			m_Array[2].Fill(value);
			m_Array[3].Fill(value);

			return *this;
		}

		constexpr Matrix& Apply(T (*func)(T))
		{
			m_Array[0].Apply(func);
			m_Array[1].Apply(func);
			m_Array[2].Apply(func);
			m_Array[3].Apply(func);

			return *this;
		}

		constexpr Matrix& Translate(const Vector<T, 3>& vec3)
		{
			m_Array[3][0] += vec3.x;
			m_Array[3][1] += vec3.y;
			m_Array[3][2] += vec3.z;

			return *this;
		}

		constexpr Matrix& Scale(const Vector<T, 3>& vec3)
		{
			m_Array[0][0] *= vec3.x; m_Array[0][1] *= vec3.x; m_Array[0][2] *= vec3.x;
			m_Array[1][0] *= vec3.y; m_Array[1][1] *= vec3.y; m_Array[1][2] *= vec3.y;
			m_Array[2][0] *= vec3.z; m_Array[2][1] *= vec3.z; m_Array[2][2] *= vec3.z;

			return *this;
		}

		inline Matrix& Rotate(T radians, const Vector<T, 3>& axis)
		{
			T c = Math::Cos(radians);
			T s = Math::Sin(radians);

			Vector<T, 3> temp((T(1) - c) * axis);

			Matrix rotateMat;
			rotateMat[0][0] = c + temp[0] * axis[0];
			rotateMat[0][1] = temp[0] * axis[1] + s * axis[2];
			rotateMat[0][2] = temp[0] * axis[2] - s * axis[1];
			rotateMat[0][3] = T(0);

			rotateMat[1][0] = temp[1] * axis[0] - s * axis[2];
			rotateMat[1][1] = c + temp[1] * axis[1];
			rotateMat[1][2] = temp[1] * axis[2] + s * axis[0];
			rotateMat[1][3] = T(0);

			rotateMat[2][0] = temp[2] * axis[0] + s * axis[1];
			rotateMat[2][1] = temp[2] * axis[1] - s * axis[0];
			rotateMat[2][2] = c + temp[2] * axis[2];
			rotateMat[2][3] = T(0);

			rotateMat[3][0] = T(0);
			rotateMat[3][1] = T(0);
			rotateMat[3][2] = T(0);
			rotateMat[3][3] = T(1);

			return *this *= rotateMat;
		}

		Matrix<T, 3, 3> AsMatrix3() const
		{
			Matrix<T, 3, 3> mat3;

			mat3[0].x = m_Array[0].x;
			mat3[0].y = m_Array[0].y;
			mat3[0].z = m_Array[0].z;

			mat3[1].x = m_Array[1].x;
			mat3[1].y = m_Array[1].y;
			mat3[1].z = m_Array[1].z;

			mat3[2].x = m_Array[2].x;
			mat3[2].y = m_Array[2].y;
			mat3[2].z = m_Array[2].z;

			return mat3;
		}

// -------------Operators-------------------------------------
	public:
		constexpr const RowType& operator[](uint32 idx) const
		{
			ATN_CORE_ASSERT(idx < Size4, "Matrix subscript out of range");
			return m_Array[idx];
		}

		constexpr RowType& operator[](uint32 idx)
		{
			ATN_CORE_ASSERT(idx < Size4, "Matrix subscript out of range");
			return m_Array[idx];
		}

		constexpr Matrix& operator+=(const Matrix& other)
		{
			m_Array[0] += other[0];
			m_Array[1] += other[1];
			m_Array[2] += other[2];
			m_Array[3] += other[3];

			return *this;
		}

		constexpr Matrix& operator-=(const Matrix& other)
		{
			m_Array[0] -= other[0];
			m_Array[1] -= other[1];
			m_Array[2] -= other[2];
			m_Array[3] -= other[3];

			return *this;
		}

		constexpr Matrix& operator*=(const Matrix& other)
		{
			return *this = Matrix(*this) * other;
		}

		constexpr Matrix& operator+=(float scalar)
		{
			m_Array[0] += scalar;
			m_Array[1] += scalar;
			m_Array[2] += scalar;
			m_Array[3] += scalar;

			return *this;
		}

		constexpr Matrix& operator-=(float scalar)
		{
			m_Array[0] -= scalar;
			m_Array[1] -= scalar;
			m_Array[2] -= scalar;
			m_Array[3] -= scalar;

			return *this;
		}

		constexpr Matrix& operator*=(float scalar)
		{
			m_Array[0] *= scalar;
			m_Array[1] *= scalar;
			m_Array[2] *= scalar;
			m_Array[3] *= scalar;

			return *this;
		}

		constexpr Matrix& operator/=(float scalar)
		{
			m_Array[0] /= scalar;
			m_Array[1] /= scalar;
			m_Array[2] /= scalar;
			m_Array[3] /= scalar;

			return *this;
		}

		constexpr Matrix operator+(const Matrix& other) const
		{
			return Matrix(*this) += other;
		}

		constexpr Matrix operator-(const Matrix& other) const
		{
			return Matrix(*this) -= other;
		}

		constexpr Matrix operator*(const Matrix& other) const
		{
			Matrix out;
			out[0] = m_Array[0] * other;
			out[1] = m_Array[1] * other;
			out[2] = m_Array[2] * other;
			out[3] = m_Array[3] * other;

			return out;
		}

		constexpr Matrix operator+(float scalar) const
		{
			return Matrix(*this) += scalar;
		}

		constexpr Matrix operator-(float scalar) const
		{
			return Matrix(*this) -= scalar;
		}

		constexpr Matrix operator*(float scalar) const
		{
			return Matrix(*this) *= scalar;
		}

		constexpr Matrix operator/(float scalar) const
		{
			return Matrix(*this) /= scalar;
		}

// -------------Static Methods-------------------------------------
	public:
		static constexpr Matrix Identity()
		{
			Matrix out(
				T(1), T(0), T(0), T(0),
				T(0), T(1), T(0), T(0),
				T(0), T(0), T(1), T(0),
				T(0), T(0), T(0), T(1));

			return out;
		}

	private:
		RowType m_Array[Size4];
	};

#undef Size4

	

// -------------Relative Functions-------------------------------------

	// For Transformation Matrices
	template <typename T>
	constexpr Matrix<T, 4, 4> AffineInverse(const Matrix<T, 4, 4>& mat)
	{
		Matrix<T, 4, 4> out;

		out[0][0] = mat[0][0]; out[0][1] = mat[1][0]; out[0][2] = mat[2][0]; out[0][3] = 0;
		out[1][0] = mat[0][1]; out[1][1] = mat[1][1]; out[1][2] = mat[2][1]; out[1][3] = 0;
		out[2][0] = mat[0][2]; out[2][1] = mat[1][2]; out[2][2] = mat[2][2]; out[2][3] = 0;

		out[3][0] = -(mat[3][0] * out[0][0] + mat[3][1] * out[1][0] + mat[3][2] * out[2][0]);
		out[3][1] = -(mat[3][0] * out[0][1] + mat[3][1] * out[1][1] + mat[3][2] * out[2][1]);
		out[3][2] = -(mat[3][0] * out[0][2] + mat[3][1] * out[1][2] + mat[3][2] * out[2][2]);
		out[3][3] = 1;

		return out;
	}

	template <typename T>
	constexpr Matrix<T, 4, 4> Inverse(const Matrix<T, 4, 4>& mat)
	{
		T Coef00 = mat[2][2] * mat[3][3] - mat[3][2] * mat[2][3];
		T Coef02 = mat[1][2] * mat[3][3] - mat[3][2] * mat[1][3];
		T Coef03 = mat[1][2] * mat[2][3] - mat[2][2] * mat[1][3];

		T Coef04 = mat[2][1] * mat[3][3] - mat[3][1] * mat[2][3];
		T Coef06 = mat[1][1] * mat[3][3] - mat[3][1] * mat[1][3];
		T Coef07 = mat[1][1] * mat[2][3] - mat[2][1] * mat[1][3];

		T Coef08 = mat[2][1] * mat[3][2] - mat[3][1] * mat[2][2];
		T Coef10 = mat[1][1] * mat[3][2] - mat[3][1] * mat[1][2];
		T Coef11 = mat[1][1] * mat[2][2] - mat[2][1] * mat[1][2];

		T Coef12 = mat[2][0] * mat[3][3] - mat[3][0] * mat[2][3];
		T Coef14 = mat[1][0] * mat[3][3] - mat[3][0] * mat[1][3];
		T Coef15 = mat[1][0] * mat[2][3] - mat[2][0] * mat[1][3];

		T Coef16 = mat[2][0] * mat[3][2] - mat[3][0] * mat[2][2];
		T Coef18 = mat[1][0] * mat[3][2] - mat[3][0] * mat[1][2];
		T Coef19 = mat[1][0] * mat[2][2] - mat[2][0] * mat[1][2];

		T Coef20 = mat[2][0] * mat[3][1] - mat[3][0] * mat[2][1];
		T Coef22 = mat[1][0] * mat[3][1] - mat[3][0] * mat[1][1];
		T Coef23 = mat[1][0] * mat[2][1] - mat[2][0] * mat[1][1];

		Vector<T, 4> Fac0(Coef00, Coef00, Coef02, Coef03);
		Vector<T, 4> Fac1(Coef04, Coef04, Coef06, Coef07);
		Vector<T, 4> Fac2(Coef08, Coef08, Coef10, Coef11);
		Vector<T, 4> Fac3(Coef12, Coef12, Coef14, Coef15);
		Vector<T, 4> Fac4(Coef16, Coef16, Coef18, Coef19);
		Vector<T, 4> Fac5(Coef20, Coef20, Coef22, Coef23);
		
		Vector<T, 4> Vec0(mat[1][0], mat[0][0], mat[0][0], mat[0][0]);
		Vector<T, 4> Vec1(mat[1][1], mat[0][1], mat[0][1], mat[0][1]);
		Vector<T, 4> Vec2(mat[1][2], mat[0][2], mat[0][2], mat[0][2]);
		Vector<T, 4> Vec3(mat[1][3], mat[0][3], mat[0][3], mat[0][3]);
		
		Vector<T, 4> Inv0(Vec1 * Fac0 - Vec2 * Fac1 + Vec3 * Fac2);
		Vector<T, 4> Inv1(Vec0 * Fac0 - Vec2 * Fac3 + Vec3 * Fac4);
		Vector<T, 4> Inv2(Vec0 * Fac1 - Vec1 * Fac3 + Vec3 * Fac5);
		Vector<T, 4> Inv3(Vec0 * Fac2 - Vec1 * Fac4 + Vec2 * Fac5);
		
		Vector<T, 4> SignA(+1, -1, +1, -1);
		Vector<T, 4> SignB(-1, +1, -1, +1);
		Matrix<T, 4, 4> Inverse;
		Inverse[0] = Inv0 * SignA;
		Inverse[1] = Inv1 * SignB;
		Inverse[2] = Inv2 * SignA;
		Inverse[3] = Inv3 * SignB;

		Vector<T, 4> Row0(Inverse[0][0], Inverse[1][0], Inverse[2][0], Inverse[3][0]);

		Vector<T, 4> Dot0(mat[0] * Row0);
		T Dot1 = (Dot0.x + Dot0.y) + (Dot0.z + Dot0.w);

		T OneOverDeterminant = static_cast<T>(1) / Dot1;

		return Inverse * OneOverDeterminant;
	}
}

namespace Athena
{
	template <>
	inline String ToString<Math::Matrix<float, 4, 4>>(const Math::Matrix<float, 4, 4>& mat)
	{
		std::stringstream stream;
		stream << "Matrix(";
		for (uint32 i = 0; i < 4; ++i)
		{
			for (uint32 j = 0; j < 4; ++j)
			{
				stream << mat[i][j] << ", ";
			}

			stream << "\n                   ";
		}
		return stream.str();
	}
}


#ifdef ATN_SIMD
#include "Athena/Math/SIMD/TypesImpl/MatrixRelational_simd.h"
#endif
