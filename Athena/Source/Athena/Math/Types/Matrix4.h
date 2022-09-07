#pragma once

#include "Matrix.h"
#include "Vector4.h"


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
			SIZE_T idx = 0;
			for (auto& row : values)
			{
				m_Array[idx] = row;
				++idx;
			}
		}

		template <typename U>
		constexpr Matrix(const Matrix<U, Size4, Size4>& other)
		{
			for (SIZE_T i = 0; i < Size4; ++i)
				m_Array[i] = static_cast<T>(other[i]);
		}


		template <typename U>
		constexpr Matrix<T, Size4, Size4>& operator=(const Matrix<U, Size4, Size4>& other)
		{
			for (SIZE_T i = 0; i < Size4; ++i)
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

		constexpr ColumnType GetColumn(SIZE_T idx) const
		{
			ATN_CORE_ASSERT(idx < Row, "Matrix subscript out of range");
			ColumnType out(m_Array[0][idx], m_Array[1][idx], m_Array[2][idx], m_Array[3][idx]);

			return out;
		}

		constexpr SIZE_T SizeOfRow() const
		{
			return Size4;
		}

		constexpr SIZE_T SizeOfColumn() const
		{
			return Size4;
		}

		constexpr iterator begin()
		{
			return iterator(m_Array, 0);
		}

		constexpr iterator end()
		{
			return iterator(m_Array, Column);
		}

		constexpr const_iterator cbegin() const
		{
			return const_iterator(m_Array, 0);
		}

		constexpr const_iterator cend() const
		{
			return const_iterator(m_Array, Column);
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
			m_Array[0][0] *= vec3.x;
			m_Array[1][1] *= vec3.y;
			m_Array[2][2] *= vec3.z;

			return *this;
		}

		inline Matrix& Rotate(T radians, const Vector<T, 3>& axis)
		{
			return *this *= Math::RotateMatrix(radians, axis);
		}

// -------------Operators-------------------------------------
	public:
		constexpr const RowType& operator[](SIZE_T idx) const
		{
			ATN_CORE_ASSERT(idx < Size4, "Matrix subscript out of range");
			return m_Array[idx];
		}

		constexpr RowType& operator[](SIZE_T idx)
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

		template <typename T>
		constexpr Matrix<T, 4, 4> operator*(const Matrix<T, 4, 4>& other) const
		{
			Matrix<T, 4, 4> out;

			for (SIZE_T i = 0; i < Size4; ++i)
			{
				for (SIZE_T j = 0; j < Size4; ++j)
				{
					out[i][j] = m_Array[i][0] * other[0][j] + 
						m_Array[i][1] * other[1][j] + 
						m_Array[i][2] * other[2][j] + 
						m_Array[i][3] * other[3][j];
				}
			}

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
			Matrix out(T(1), T(0), T(0), T(0),
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
}


#ifdef ATN_SIMD
#include "Athena/Math/SIMD/Types/MatrixRelational_simd.h"
#endif

