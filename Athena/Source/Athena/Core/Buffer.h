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

		Buffer(void* data, uint64 size)
			: m_Data((byte*)data), m_Size(size)
		{
			
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

			m_Data = (byte*)malloc(size);
			m_Size = size;
		}

		void Release()
		{
			free(m_Data);
			m_Data = nullptr;
			m_Size = 0;
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

	protected:
		byte* m_Data = nullptr;
		uint64 m_Size = 0;
	};


	class UniqueBuffer : public Buffer
	{
	public:
		UniqueBuffer(void* data, uint64 size)
			: Buffer(data, size)
		{

		}

		UniqueBuffer(uint64 size)
			: Buffer(size)
		{

		}

		UniqueBuffer(const UniqueBuffer& other) = delete;
		UniqueBuffer& operator=(const UniqueBuffer& other) = delete;

		UniqueBuffer(UniqueBuffer&& other) noexcept
			: Buffer(other.Data(), other.Size())
		{
			other.m_Data = nullptr;
			other.m_Size = 0;
		}

		UniqueBuffer& operator=(UniqueBuffer&& other) noexcept
		{
			if (*this != other)
			{
				m_Data = other.m_Data;
				m_Size = other.m_Size;
				other.m_Data = nullptr;
				other.m_Size = 0;
			}

			return *this;
		}

		~UniqueBuffer()
		{
			Release();
		}

	private:
		using Buffer::Allocate;
		using Buffer::Release;
	};
}
