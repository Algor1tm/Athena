#pragma once

#include "Athena/Core/Core.h"
#include "Vector.h"

#include <sstream>


namespace Athena::Math
{
	// Column - size of columns
	// Row - size of rows
	template <typename T, SIZE_T Column, SIZE_T Row>
	class Matrix
	{
	public:
		using RowType = Vector<T, Row>;
		using ColumnType = Vector<T, Column>;

	public:
		using iterator = VectorIterator<Vector<T, Row>, Column>;
		using const_iterator = VectorConstIterator<Vector<T, Row>, Column>;

// -------------Constructors-------------------------------------
	public:
		constexpr Matrix() = default;

		explicit constexpr Matrix(T scalar)
		{
			Fill(scalar);
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

		constexpr Matrix(const RowType& vec)
		{
			static_assert(Column == 1, "Cannot initialize matrix with vector");
			m_Array[0] = vec;
		}

		template <typename U>
		constexpr Matrix(const Matrix<U, Column, Row>& other)
		{
			for (SIZE_T i = 0; i < Column; ++i)
				m_Array[i] = static_cast<T>(other[i]);
		}


		template <typename U>
		constexpr Matrix<T, Column, Row>& operator=(const Matrix<U, Column, Row>& other)
		{
			for (SIZE_T i = 0; i < Column; ++i)
				m_Array[i] = static_cast<T>(other[i]);
		}

// -------------Type Casts-------------------------------------
	public:
		constexpr explicit operator Vector<T, Row>() const
		{
			static_assert(Column == 1, "Cannot convert matrix to vector");
			return m_Array[0];
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

// -------------Operators-------------------------------------
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
				m_Array[i] /= scalar;
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

// -------------Static Methods-------------------------------------
	public:
		static constexpr Matrix Identity()
		{
			Matrix out(static_cast<T>(0));

			constexpr SIZE_T min = Min(Column, Row);
			for (SIZE_T i = 0; i < min; ++i)
				out[i][i] = static_cast<T>(1);

			return out;
		}

	private:
		RowType m_Array[Column];
	};



// -------------Relative Functions-------------------------------------

	template <typename T, SIZE_T Column, SIZE_T Row>
	constexpr Matrix<T, Column, Row> operator+(float scalar, const Matrix<T, Column, Row>& mat)
	{
		return mat + scalar;
	}

	template <typename T, SIZE_T Column, SIZE_T Row>
	constexpr Matrix<T, Column, Row> operator*(float scalar, const Matrix<T, Column, Row>& mat)
	{
		return mat * scalar;
	}

	template <typename T, SIZE_T vecRow, SIZE_T Column, SIZE_T Row>
	constexpr Vector<T, Row> operator*(
		const Vector<T, vecRow>& vec, const Matrix<T, Column, Row>& mat)
	{
		static_assert(vecRow == Column, "Invalid Vector Matrix multiplication");

		Vector<T, Row> out(static_cast<T>(0));
		for (SIZE_T i = 0; i < Column; i++)
		{
			for (SIZE_T j = 0; j < Row; ++j)
				out[j] += vec[i] * mat[i][j];
		}
		return out;
	}

	template <typename T, SIZE_T Column, SIZE_T Row>
	constexpr Matrix<T, Row, Column> Transpose(const Matrix<T, Column, Row>& mat)
	{
		Matrix<T, Row, Column> out;
		for (SIZE_T i = 0; i < Column; ++i)
		{
			for (SIZE_T j = 0; j < Row; ++j)
				out[j][i] = mat[i][j];
		}
		return out;
	}
}


namespace Athena
{
	template <typename T, SIZE_T Column, SIZE_T Row>
	inline String ToString(const Math::Matrix<T, Column, Row>& mat)
	{
		std::stringstream stream;
		stream << "Matrix(";
		for (SIZE_T i = 0; i < Column; ++i)
		{
			for (SIZE_T j = 0; j < Row; ++j)
			{
				stream << mat[i][j] << ", ";
			}
			if (i == Column - 1)
				stream << "Column = " << Column << ", Row = " << Row << ")";
			else
				stream << "\n       ";
		}
		return stream.str();
	}
}

