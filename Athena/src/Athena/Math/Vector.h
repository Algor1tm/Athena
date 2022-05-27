#pragma once

#include "atnpch.h"
#include "Athena/Core.h"


namespace Athena
{
	template <typename Ty, size_t Size>
	class VectorConstIterator
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = ptrdiff_t;
		using pointer = const Ty*;
		using reference = const Ty&;

	public:
		constexpr VectorConstIterator()
			: m_Ptr(nullptr), m_Idx(0) {}

		constexpr explicit VectorConstIterator(pointer Ptr, size_t Off = 0)
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
			m_Idx += static_cast<size_t>(Off);
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
				ATN_CORE_ASSERT(m_Idx >= size_t{ 0 } - static_cast<size_t>(Off), "Cannot seek vector iterator before begin");

			if (Off > 0)
				ATN_CORE_ASSERT(Size - m_Idx >= static_cast<size_t>(Off), "Cannot seek vector iterator after end");
		}

	private:
		pointer m_Ptr;
		size_t m_Idx;
	};


	template <class Ty, size_t Size>
	constexpr VectorConstIterator<Ty, Size> operator+(
		const ptrdiff_t Off, VectorConstIterator<Ty, Size> Iter)
	{
		return Iter += Off;
	}


	template <class Ty, size_t Size>
	class VectorIterator: public VectorConstIterator<Ty, Size>
	{
	public:
		using Base = VectorConstIterator<Ty, Size>;

		using iterator_category = std::random_access_iterator_tag;
		using difference_type = ptrdiff_t;
		using pointer = Ty*;
		using reference = Ty&;

	public:
		constexpr VectorIterator() = default;

		constexpr explicit VectorIterator(pointer Ptr, size_t Off = 0)
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

	template <class _Ty, size_t _Size>
	constexpr VectorIterator<_Ty, _Size> operator+(
		const ptrdiff_t Off, VectorIterator<_Ty, _Size> Next)
	{
		return Next += Off;
	}


	template <typename Ty, size_t Size>
	class Vector
	{
	public:
		using iterator = VectorIterator<Ty, Size>;
		using const_iterator = VectorConstIterator<Ty, Size>;

	public:
		constexpr Vector() = default;

		constexpr Vector(Ty value)
		{
			Fill(value);
		}

		constexpr Vector(const std::initializer_list<Ty>& values)
		{
			ATN_CORE_ASSERT(values.size() == Size, 
				"Cannot initialize vector with initializer list");

			size_t iter = 0;
			for (auto val : values)
			{
				m_Array[iter] = val;
				++iter;
			}
		}

		constexpr Vector(const Vector& other) = default;

		template <typename U>
		constexpr Vector<Ty, Size>(const Vector<U, Size>& other)
		{
			static_assert(std::is_convertible<U, Ty>::value,
				"Vector initialization error: Vectors are not convertible");

			for (size_t i = 0; i < Size; ++i)
				m_Array[i] = static_cast<Ty>(other[i]);
		}

		constexpr Vector& operator=(const Vector& other) = default;

		template <typename U>
		constexpr Vector<Ty, Size>& operator=(const Vector<U, Size>& other)
		{
			static_assert(std::is_convertible<U, Ty>::value,
				"Vector assignment error: Vectors are not convertible");

			for (size_t i = 0; i < Size; ++i)
				m_Array[i] = static_cast<Ty>(other[i]);

			return *this;
		}

		constexpr void Fill(Ty value)
		{
			for (size_t i = 0; i < Size; ++i)
				m_Array[i] = value;
		}

		constexpr const Ty& operator[](size_t idx) const
		{
			ATN_CORE_ASSERT(idx < Size, "Vector subscript out of range");
			return m_Array[idx];
		}

		constexpr Ty& operator[](size_t idx)
		{
			ATN_CORE_ASSERT(idx < Size, "Vector subscript out of range");
			return m_Array[idx];
		}

		constexpr size_t GetSize() const noexcept
		{
			return Size;
		}

		constexpr Ty* Data() noexcept
		{
			return m_Array;
		}

		constexpr Ty* Data() const noexcept
		{
			return m_Array;
		}

		constexpr iterator begin() noexcept
		{
			return iterator(m_Array, 0);
		}

		constexpr iterator end() noexcept
		{
			return iterator(m_Array, Size);
		}

		constexpr const_iterator cbegin() const noexcept
		{
			return const_iterator(m_Array, 0);
		}

		constexpr const_iterator cend() const noexcept
		{
			return const_iterator(m_Array, Size);
		}

		constexpr Vector& Apply(Ty (*func)(Ty))
		{
			for (size_t i = 0; i < Size; ++i)
				m_Array[i] = func(m_Array[i]);
			return *this;
		}

		constexpr Ty GetSqrLength() const noexcept
		{
			Ty out;
			for (size_t i = 0; i < Size; ++i)
				out += m_Array[i] * m_Array[i];
			return out;
		}

		constexpr float GetLength() const noexcept
		{
			return std::sqrt(static_cast<float>(GetSqrLength()));
		}

		constexpr Vector& Normalize()
		{
			float length = GetLength();
			return length == 0 ? *this : *this /= static_cast<Ty>(length);
		}

		constexpr Vector GetNormalized() const
		{
			float length = GetLength();
			return length == 0 ? Vector(*this) : Vector(*this) /= static_cast<Ty>(length);
		}

	public:
		constexpr Vector& operator+=(const Vector& other) noexcept
		{
			for (size_t i = 0; i < Size; ++i)
				m_Array[i] += other.m_Array[i];
			return *this;
		}

		constexpr Vector& operator-=(const Vector& other) noexcept
		{
			for (size_t i = 0; i < Size; ++i)
				m_Array[i] -= other.m_Array[i];
			return *this;
		}

		constexpr Vector& operator+=(Ty scalar) noexcept
		{
			for (size_t i = 0; i < Size; ++i)
				m_Array[i] += scalar;
			return *this;
		}

		constexpr Vector& operator-=(Ty scalar) noexcept
		{
			for (size_t i = 0; i < Size; ++i)
				m_Array[i] -= scalar;
			return *this;
		}

		constexpr Vector& operator*=(Ty scalar) noexcept
		{
			for (size_t i = 0; i < Size; ++i)
				m_Array[i] *= scalar;
			return *this;
		}

		constexpr Vector& operator/=(Ty scalar)
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector operation error: dividing by zero");
			for (size_t i = 0; i < Size; ++i)
				m_Array[i] /= scalar;
			return *this;
		}

		constexpr Vector operator+(const Vector& other) const noexcept
		{
			return Vector(*this) += other;
		}

		constexpr Vector operator-(const Vector& other) const noexcept
		{
			return Vector(*this) -= other;
		}

		constexpr Vector operator+(Ty scalar) const noexcept
		{
			return Vector(*this) += scalar;
		}

		constexpr Vector operator-(Ty scalar) const noexcept
		{
			return Vector(*this) -= scalar;
		}

		constexpr Vector operator*(Ty scalar) const noexcept
		{
			return Vector(*this) *= scalar;
		}

		constexpr Vector operator/(Ty scalar) const
		{
			ATN_CORE_ASSERT(scalar != 0, "Vector operation error: dividing by zero");
			return Vector(*this) /= scalar;
		}

		constexpr Vector operator-() const noexcept
		{
			Vector out(*this);
			for (size_t i = 0; i < Size; ++i)
				out.m_Array[i] = -out.m_Array[i];
			return out;
		}

		constexpr bool operator==(const Vector& other) const noexcept
		{
			bool out = true;
			for (size_t i = 0; i < Size; ++i)
				out = out && m_Array[i] == other.m_Array[i];
			return out;
		}

		constexpr bool operator!=(const Vector& other) const noexcept
		{
			return !(*this == other);
		}

	private:
		Ty m_Array[Size];
	};


	template <typename Ty, size_t Size>
	constexpr Vector<Ty, Size> operator+(Ty scalar, const Vector<Ty, Size>& vec)
	{
		return vec + scalar;
	}

    template <typename Ty, size_t Size>
	constexpr Vector<Ty, Size> operator*(Ty scalar, const Vector<Ty, Size>& vec) noexcept
	{
		return vec * scalar;
	}

	template <typename Ty, size_t Size>
	constexpr Ty Dot(const Vector<Ty, Size>& Left, const Vector<Ty, Size>& Right)  noexcept
	{
		Ty out;
		for (size_t i = 0; i < Size; ++i)
			out += Left[i] * Right[i];
		return out;
	}

	template <typename Ty, size_t Size>
	constexpr float Distance(const Vector<Ty, Size>& Left, const Vector<Ty, Size>& Right)
	{
		return (Left - Right).GetLength();
	}

	template <typename Ty, size_t Size>
	constexpr void Swap(Vector<Ty, Size>& Left, Vector<Ty, Size>& Right)
	{
		for (size_t i = 0; i < Size; ++i)
			std::swap(Left[i], Right[i]);
	}


	template <typename Ty, size_t Size>
	constexpr std::string ToString(const Vector<Ty, Size>& vec)
	{
		std::string out = "Vector" + std::to_string(vec.GetSize()) + "(";
		for (size_t i = 0; i < vec.GetSize() - 1; ++i)
			out += std::to_string(vec[i]) + ", ";
		out += std::to_string(vec[vec.GetSize() - 1]) + ")";
		return out;
	}
}
