#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"

#include "Athena/Math/Utils/Common.h"


namespace Athena::Math
{
	template <typename T, SIZE_T _Size>
	class Vector;

	// Forward declaration (VectorRelational.h)
	template <typename T, SIZE_T Size>
	constexpr T Dot(const Vector<T, Size>& left, const Vector<T, Size>& right);
}

namespace Athena
{
	template <typename T, SIZE_T Size>
	class VectorConstIterator
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = ptrdiff_t;
		using pointer = const T*;
		using reference = const T&;

	public:
		constexpr VectorConstIterator()
			: m_Ptr(nullptr), m_Idx(0) {}

		constexpr explicit VectorConstIterator(pointer Ptr, SIZE_T Off = 0)
			: m_Ptr(Ptr), m_Idx(Off) {}

		constexpr reference operator*() const
		{
			return *operator->();
		}

		constexpr pointer operator->() const
		{
			ATN_CORE_ASSERT(m_Ptr, "Cannot dereference value-initialized vector iterator");
			ATN_CORE_ASSERT(m_Idx < Size, "Cannot dereference out of range vector iterator");
			return m_Ptr + m_Idx;
		}

		constexpr VectorConstIterator& operator++()
		{
			ATN_CORE_ASSERT(m_Ptr, "Cannot increment value-initialized vector iterator");
			ATN_CORE_ASSERT(m_Idx < Size, "Cannot increment vector iterator past end");
			++m_Idx;
			return *this;
		}

		constexpr VectorConstIterator operator++(int)
		{
			VectorConstIterator tmp = *this;
			++* this;
			return tmp;
		}

		constexpr VectorConstIterator& operator--()
		{
			ATN_CORE_ASSERT(m_Ptr, "Cannot decrement value-initialized array iterator");
			ATN_CORE_ASSERT(m_Idx != 0, "Cannot decrement vector iterator before begin");
			--m_Idx;
			return *this;
		}

		constexpr VectorConstIterator operator--(int)
		{
			VectorConstIterator tmp = *this;
			--* this;
			return tmp;
		}

		constexpr VectorConstIterator& operator+=(const ptrdiff_t Off)
		{
			VerifyOffset(Off);
			m_Idx += static_cast<SIZE_T>(Off);
			return *this;
		}

		constexpr VectorConstIterator operator+(const ptrdiff_t Off) const
		{
			VectorConstIterator tmp = *this;
			return tmp += Off;
		}

		constexpr VectorConstIterator& operator-=(const ptrdiff_t Off)
		{
			return *this += -Off;
		}

		constexpr VectorConstIterator operator-(const ptrdiff_t Off) const
		{
			VectorConstIterator tmp = *this;
			return tmp -= Off;
		}

		constexpr ptrdiff_t operator-(const VectorConstIterator& other) const
		{
			Compat(other);
			return static_cast<ptrdiff_t>(m_Idx - other.m_Idx);
		}

		constexpr reference operator[](const ptrdiff_t Off) const
		{
			return *(*this + Off);
		}

		constexpr bool operator==(const VectorConstIterator& other) const
		{
			Compat(other);
			return m_Idx == other.m_Idx;
		}

		constexpr bool operator!=(const VectorConstIterator& other) const
		{
			return !(*this == other);
		}

		constexpr bool operator<(const VectorConstIterator& other) const
		{
			Compat(other);
			return m_Idx < other.m_Idx;
		}

		constexpr bool operator>(const VectorConstIterator& other) const
		{
			return other < *this;
		}

		constexpr bool operator<=(const VectorConstIterator& other) const
		{
			return !(other < *this);
		}

		constexpr bool operator>=(const VectorConstIterator& other) const
		{
			return !(*this < other);
		}

	private:
		constexpr void Compat(const VectorConstIterator& other) const
		{
			ATN_CORE_ASSERT(m_Ptr == other.m_Ptr, "Vector iterators incompatible");
		}

		constexpr void VerifyOffset(const ptrdiff_t Off) const
		{
			if (Off != 0)
				ATN_CORE_ASSERT(m_Ptr, "Cannot seek value-initialized vector iterator");

			if (Off < 0)
				ATN_CORE_ASSERT(m_Idx >= SIZE_T{ 0 } - static_cast<SIZE_T>(Off), "Cannot seek vector iterator before begin");

			if (Off > 0)
				ATN_CORE_ASSERT(Size - m_Idx >= static_cast<SIZE_T>(Off), "Cannot seek vector iterator after end");
		}

	private:
		pointer m_Ptr;
		SIZE_T m_Idx;
	};


	template <class T, SIZE_T Size>
	constexpr VectorConstIterator<T, Size> operator+(
		const ptrdiff_t Off, VectorConstIterator<T, Size> Iter)
	{
		return Iter += Off;
	}


	template <class T, SIZE_T Size>
	class VectorIterator : public VectorConstIterator<T, Size>
	{
	public:
		using Base = VectorConstIterator<T, Size>;

		using iterator_category = std::random_access_iterator_tag;
		using difference_type = ptrdiff_t;
		using pointer = T*;
		using reference = T&;

	public:
		constexpr VectorIterator() = default;

		constexpr explicit VectorIterator(pointer Ptr, SIZE_T Off = 0)
			: Base(Ptr, Off) {}

		constexpr reference operator*() const
		{
			return const_cast<reference>(Base::operator*());
		}

		constexpr pointer operator->() const
		{
			return const_cast<pointer>(Base::operator->());
		}

		constexpr VectorIterator& operator++()
		{
			Base::operator++();
			return *this;
		}

		constexpr VectorIterator operator++(int)
		{
			VectorIterator tmp = *this;
			Base::operator++();
			return tmp;
		}

		constexpr VectorIterator& operator--()
		{
			Base::operator--();
			return *this;
		}

		constexpr VectorIterator operator--(int)
		{
			VectorIterator tmp = *this;
			Base::operator--();
			return tmp;
		}

		constexpr VectorIterator& operator+=(const ptrdiff_t Off)
		{
			Base::operator+=(Off);
			return *this;
		}

		constexpr VectorIterator operator+(const ptrdiff_t Off) const
		{
			VectorIterator tmp = *this;
			return tmp += Off;
		}

		constexpr VectorIterator& operator-=(const ptrdiff_t Off)
		{
			Base::operator-=(Off);
			return *this;
		}

		using Base::operator-;

		constexpr VectorIterator operator-(const ptrdiff_t Off) const
		{
			VectorIterator tmp = *this;
			return tmp -= Off;
		}

		constexpr reference operator[](const ptrdiff_t Off) const
		{
			return const_cast<reference>(Base::operator[](Off));
		}
	};

	template <class T, SIZE_T Size>
	constexpr VectorIterator<T, Size> operator+(
		const ptrdiff_t Off, VectorIterator<T, Size> Next)
	{
		return Next += Off;
	}
}

namespace Athena::Math
{
	template <typename T, SIZE_T _Size>
	class Vector
	{
	public:
		using iterator = VectorIterator<T, _Size>;
		using const_iterator = VectorConstIterator<T, _Size>;

// -------------Constructors-------------------------------------
	public:
		constexpr Vector() = default;

		explicit constexpr Vector(T scalar)
		{
			Fill(scalar);
		}

		constexpr Vector(const std::initializer_list<T>& values)
		{
			ATN_CORE_ASSERT(values.size() == _Size,
				"Cannot initialize vector with initializer list");

			SIZE_T iter = 0;
			for (auto val : values)
			{
				m_Array[iter] = val;
				++iter;
			}
		}


		constexpr Vector(const Vector& other) = default;
		constexpr Vector& operator=(const Vector& other) = default;


		template <typename U>
		constexpr Vector<T, _Size>(const Vector<U, _Size>& other)
		{
			for (SIZE_T i = 0; i < _Size; ++i)
				m_Array[i] = static_cast<T>(other[i]);
		}


		template <typename U>
		constexpr Vector<T, _Size>& operator=(const Vector<U, _Size>& other)
		{
			for (SIZE_T i = 0; i < _Size; ++i)
				m_Array[i] = static_cast<T>(other[i]);

			return *this;
		}

// -------------Public Methods-------------------------------------
	public:
		constexpr SIZE_T Size() const
		{
			return _Size;
		}

		constexpr T* Data()
		{
			return m_Array;
		}

		constexpr const T* Data() const
		{
			return m_Array;
		}

		constexpr iterator begin()
		{
			return iterator(m_Array, 0);
		}

		constexpr iterator end()
		{
			return iterator(m_Array, _Size);
		}

		constexpr const_iterator cbegin() const
		{
			return const_iterator(m_Array, 0);
		}

		constexpr const_iterator cend() const
		{
			return const_iterator(m_Array, _Size);
		}

		constexpr void Fill(T value)
		{
			for (SIZE_T i = 0; i < _Size; ++i)
				m_Array[i] = value;
		}

		constexpr Vector& Apply(T(*func)(T))
		{
			for (SIZE_T i = 0; i < _Size; ++i)
				m_Array[i] = func(m_Array[i]);
			return *this;
		}

		constexpr T SqrLength() const
		{
			return Dot(*this, *this);
		}

		constexpr float Length() const
		{
			return Math::Sqrt(SqrLength());
		}

		constexpr Vector& Normalize()
		{
			float length = Length();
			return length == 0 ? *this : *this /= static_cast<T>(length);
		}

		constexpr Vector GetNormalized() const
		{
			float length = Length();
			return length == 0 ? Vector(*this) : Vector(*this) /= static_cast<T>(length);
		}

// -------------Operators-------------------------------------
	public:
		constexpr T operator[](SIZE_T idx) const
		{
			ATN_CORE_ASSERT(idx < _Size, "Vector subscript out of range");
			return m_Array[idx];
		}

		constexpr T& operator[](SIZE_T idx)
		{
			ATN_CORE_ASSERT(idx < _Size, "Vector subscript out of range");
			return m_Array[idx];
		}

		constexpr Vector& operator+=(const Vector& other)
		{
			for (SIZE_T i = 0; i < _Size; ++i)
				m_Array[i] += other.m_Array[i];
			return *this;
		}

		constexpr Vector& operator-=(const Vector& other)
		{
			for (SIZE_T i = 0; i < _Size; ++i)
				m_Array[i] -= other.m_Array[i];
			return *this;
		}

		constexpr Vector& operator*=(const Vector& other)
		{
			for (SIZE_T i = 0; i < _Size; ++i)
				m_Array[i] *= other.m_Array[i];
			return *this;
		}

		constexpr Vector& operator/=(const Vector& other)
		{
			for (SIZE_T i = 0; i < _Size; ++i)
				m_Array[i] /= other.m_Array[i];
			return *this;
		}

		constexpr Vector& operator+=(T scalar)
		{
			for (SIZE_T i = 0; i < _Size; ++i)
				m_Array[i] += scalar;
			return *this;
		}

		constexpr Vector& operator-=(T scalar)
		{
			for (SIZE_T i = 0; i < _Size; ++i)
				m_Array[i] -= scalar;
			return *this;
		}

		constexpr Vector& operator*=(T scalar)
		{
			for (SIZE_T i = 0; i < _Size; ++i)
				m_Array[i] *= scalar;
			return *this;
		}

		constexpr Vector& operator/=(T scalar)
		{
			for (SIZE_T i = 0; i < _Size; ++i)
				m_Array[i] /= scalar;
			return *this;
		}

		constexpr Vector operator+(const Vector& other) const
		{
			return Vector(*this) += other;
		}

		constexpr Vector operator-(const Vector& other) const
		{
			return Vector(*this) -= other;
		}

		constexpr Vector operator*(const Vector& other) const
		{
			return Vector(*this) *= other;
		}

		constexpr Vector operator/(const Vector& other) const
		{
			return Vector(*this) /= other;
		}

		constexpr Vector operator+(T scalar) const 
		{
			return Vector(*this) += scalar;
		}

		constexpr Vector operator-(T scalar) const 
		{
			return Vector(*this) -= scalar;
		}

		constexpr Vector operator*(T scalar) const 
		{
			return Vector(*this) *= scalar;
		}

		constexpr Vector operator/(T scalar) const
		{
			return Vector(*this) /= scalar;
		}

		constexpr Vector operator-() const 
		{
			Vector out(*this);
			for (SIZE_T i = 0; i < _Size; ++i)
				out.m_Array[i] = -out.m_Array[i];
			return out;
		}

		constexpr bool operator==(const Vector& other) const 
		{
			bool out = true;
			for (SIZE_T i = 0; i < _Size; ++i)
				out = out && m_Array[i] == other.m_Array[i];
			return out;
		}

		constexpr bool operator!=(const Vector& other) const 
		{
			return !(*this == other);
		}

	private:
		T m_Array[_Size];
	};
}
