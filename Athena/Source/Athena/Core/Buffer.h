#pragma once
#include "Athena/Core/Core.h"


namespace Athena
{
	class Buffer
	{
	public:
		Buffer() = default;

		Buffer(uint64 size)
		{
			Allocate(size);
		}

		static Buffer Copy(const Buffer& other)
		{
			Buffer result(other.m_Size);
			memcpy(result.m_Data, other.m_Data, other.m_Size);
			return result;
		}

		static Buffer Copy(const void* data, uint64 size)
		{
			Buffer result(size);
			memcpy(result.m_Data, data, size);
			return result;
		}

		static Buffer Move(const void* data, uint64 size)
		{
			Buffer result;
			result.m_Data = (byte*)data;
			result.m_Size = size;
			return result;
		}

		byte* Data() 
		{
			return m_Data; 
		}

		const byte* Data() const 
		{
			return m_Data; 
		}

		uint64 Size() const 
		{
			return m_Size; 
		}

		void Allocate(uint64 size)
		{
			Release();

			if(size != 0)
				m_Data = (byte*)malloc(size);

			m_Size = size;
		}

		void Release()
		{
			free(m_Data);
			m_Data = nullptr;
			m_Size = 0;
		}

		void Write(void* data, uint64 size, uint64 offset = 0)
		{
			memcpy(m_Data + offset, data, size);
		}

		template<typename T>
		T* As()
		{
			return (T*)m_Data;
		}

		operator bool() const
		{
			return m_Data == nullptr;
		}

		bool operator==(const Buffer& other) const
		{
			return m_Data == other.m_Data && m_Size == other.m_Size;
		}

		bool operator!=(const Buffer other) const
		{
			return !(*this == other);
		}

	private:
		byte* m_Data = nullptr;
		uint64 m_Size = 0;
	};
}
