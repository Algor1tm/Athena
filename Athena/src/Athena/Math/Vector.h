#pragma once

#include "atnpch.h"
#include "Athena/Core.h"


namespace Athena
{
	template <typename T, size_t Size>
	class Vector
	{
	public:
		using value_type = T;
		using container = std::array<T, Size>;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*;
		using const_pointer = const T*;
		using size_type = size_t;
		using difference_type = std::ptrdiff_t;

	public:
		using iterator = typename container::iterator;
		using const_iterator = typename container::const_iterator;
		using reverse_iterator = typename container::reverse_iterator;
		using const_reverse_iterator = typename container::const_reverse_iterator;

	public:
		constexpr Vector(value_type value = 0)
		{
			m_Array.fill(value);
		}

		constexpr Vector(const std::initializer_list<value_type>& values)
		{
			ATN_CORE_ASSERT(values.size() == Size,
				"Vector initialization error: size of initializer list must be equal to vector size");
			
			if (values.size() > 0)
			{
				size_type iter = 0;
				for (auto val : values)
				{
					m_Array[iter] = val;
					++iter;
				}
			}
		}

		constexpr Vector(const Vector& other) = default;

		template <typename U>
		constexpr Vector<T, Size>(const Vector<U, Size>& other)
		{
			static_assert(std::is_convertible<U, T>::value,
			"Vector initialization error: could not initialize Vector from another");

			for (size_type i = 0; i < Size; ++i)
				m_Array[i] = static_cast<T>(other[i]);
		}

		constexpr Vector(Vector&& other) noexcept = default;

		template <typename U>
		constexpr Vector<T, Size>(Vector<U, Size>&& other) noexcept
		{
			static_assert(std::is_convertible<U, T>::value,
				"Vector initialization error: could not initialize Vector from another");

			for (size_type i = 0; i < Size; ++i)
				m_Array[i] = std::move(static_cast<T>(other[i]));
		}

		constexpr value_type operator[](size_type idx) const
		{
			ATN_CORE_ASSERT(idx < Size, "Vector indexing error: Index out of range");
			return m_Array[idx];
		}

		constexpr reference operator[](size_type idx)
		{
			ATN_CORE_ASSERT(idx < Size, "Vector indexing error: Index out of range");
			return m_Array[idx];
		}
		
		constexpr size_type GetSize() const
		{
			return Size;
		}

	private:
		container m_Array;
	};

	template<typename T, size_t Size>
	std::string ToString(const Vector<T, Size>& vec)
	{
		std::string out = "Vector" + std::to_string(vec.GetSize()) + "(";
		for (size_t i = 0; i < vec.GetSize() - 1; ++i)
			out += std::to_string(vec[i]) + ", ";
		out += std::to_string(vec[vec.GetSize() - 1]) + ")";
		return out;
	}


	template <typename T>
	class Vector<T, 2>
	{

	};


	template <typename T>
	class Vector<T, 3>
	{

	};


	template <typename T>
	class Vector<T, 4>
	{

	};


	typedef Vector<float, 2> Vector2;
	typedef Vector<int, 2> Vector2i;
	typedef Vector<double, 2> Vector2d;

	typedef Vector<float, 3> Vector3;
	typedef Vector<int, 3> Vector3i;
	typedef Vector<double, 3> Vector3d;

	typedef Vector<float, 4> Vector4;
	typedef Vector<int, 4> Vector4i;
	typedef Vector<double, 4> Vector4d;
}
