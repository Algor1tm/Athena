#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"
#include "Athena/Math/SIMD/Types/Vector4_float.h"


namespace Athena
{
	// ROW-MAJOR
	// Column - size of column
	// Row - size of row
	template <typename T, SIZE_T Column, SIZE_T Row>
	class Matrix
	{
	public:
		using RowType = Vector<T, Row>;
		using ColumnType = Vector<T, Column>;

	public:
		using iterator = VectorIterator<Vector<T, Row>, Column>;
		using const_iterator = VectorConstIterator<Vector<T, Row>, Column>;

	// Constructors
	public:
		constexpr Matrix() = default;

		template <typename X>
		explicit constexpr Matrix(X scalar)
		{
			Fill(static_cast<T>(scalar));
		}

		constexpr Matrix(const std::initializer_list<RowType>& values)
		{
			ATN_CORE_ASSERT(values.size() == Column, "Invalid initializer list");
			SIZE_T idx = 0;
			for (auto& row : values)
			{
				m_Array[idx] = row;
				++idx;
			}
		}

		constexpr Matrix(const Matrix& other) = default;
		constexpr Matrix& operator=(const Matrix& other) = default;

		constexpr Matrix(const RowType& vec)
		{
			static_assert(Column == 1, "Cannot initialize matrix with vector");
			m_Array[0] = vec;
		}

		constexpr explicit operator Vector<T, Row>() const
		{
			static_assert(Column == 1, "Cannot convert matrix to vector");
			return m_Array[0];
		}

		template <typename U>
		constexpr Matrix(const Vector<U, Row>& vec)
		{
			static_assert(Column == 1, "Cannot initialize matrix with vector");
			m_Array[0] = vec;
		}

		template <typename U>
		constexpr Matrix(const Matrix<U, Column, Row>& other)
		{
			for (SIZE_T i = 0; i < Column; ++i)
				m_Array[i] = other[i];
		}


		template <typename U>
		constexpr Matrix<T, Column, Row>& operator=(const Matrix<U, Column, Row>& other)
		{
			for (SIZE_T i = 0; i < Column; ++i)
				m_Array[i] = other[i];
		}

	// Public Methods
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
			ColumnType out;
			for (SIZE_T i = 0; i < Column; ++i)
				out[i] = m_Array[i][idx];
			return out;
		}

		constexpr SIZE_T SizeOfRow() const
		{
			return Row;
		}

		constexpr SIZE_T SizeOfColumn() const
		{
			return Column;
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
			for (SIZE_T i = 0; i < Column; ++i)
				m_Array[i].Fill(value);
			return *this;
		}

		constexpr Matrix& Apply(T(*func)(T))
		{
			for (SIZE_T i = 0; i < Column; ++i)
				m_Array[i].Apply(func);
			return *this;
		}

	// Operators
	public:
		constexpr const RowType& operator[](SIZE_T idx) const
		{
			ATN_CORE_ASSERT(idx < Column, "Matrix subscript out of range");
			return m_Array[idx];
		}

		constexpr RowType& operator[](SIZE_T idx)
		{
			ATN_CORE_ASSERT(idx < Column, "Matrix subscript out of range");
			return m_Array[idx];
		}

		constexpr Matrix& operator+=(const Matrix& other)
		{
			for (SIZE_T i = 0; i < Column; ++i)
				m_Array[i] += other[i];
			return *this;
		}

		constexpr Matrix& operator-=(const Matrix& other)
		{
			for (SIZE_T i = 0; i < Column; ++i)
				m_Array[i] -= other[i];
			return *this;
		}

		constexpr Matrix& operator+=(float scalar)
		{
			for (SIZE_T i = 0; i < Column; ++i)
				m_Array[i] += scalar;
			return *this;
		}

		constexpr Matrix& operator-=(float scalar)
		{
			for (SIZE_T i = 0; i < Column; ++i)
				m_Array[i] -= scalar;
			return *this;
		}

		constexpr Matrix& operator*=(float scalar)
		{
			for (SIZE_T i = 0; i < Column; ++i)
				m_Array[i] *= scalar;
			return *this;
		}

		constexpr Matrix& operator/=(float scalar)
		{
			for (SIZE_T i = 0; i < Column; ++i)
				for (SIZE_T j = 0; j < Row; ++j)
					m_Array[i][j] /= scalar;
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

		template <SIZE_T OtherColumn, SIZE_T OtherRow>
		constexpr Matrix<T, Column, OtherRow> operator*(const Matrix<T, OtherColumn, OtherRow>& other) const
		{
			static_assert(Row == OtherColumn, "Invalid Matrix multiplication");

			Matrix<T, Column, OtherRow> out(static_cast<T>(0));
			for (SIZE_T i = 0; i < Column; i++)
			{
				for (SIZE_T j = 0; j < OtherRow; j++)
				{
					for (SIZE_T k = 0; k < Row; ++k)
						out[i][j] += m_Array[i][k] * other[k][j];
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

	// Static Methods
	public:
		static constexpr Matrix<T, Column, Row> Identity()
		{
			Matrix out(static_cast<T>(0));

			constexpr SIZE_T min = std::min(Column, Row);
			for (SIZE_T i = 0; i < min; ++i)
				out[i][i] = static_cast<T>(1);

			return out;
		}

	private:
		RowType m_Array[Column];
	};
}
