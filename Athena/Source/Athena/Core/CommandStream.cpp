#include "CommandStream.h"


namespace Athena
{
	CommandStream::CommandStream(uint64 sizeInBytes)
		: m_Size(sizeInBytes)
	{
		m_Buffer = new byte[sizeInBytes];
		m_BufferEnd = m_Buffer;
	}

	CommandStream::~CommandStream()
	{
		delete[] m_Buffer;
	}

	CommandStream::CommandStream(CommandStream&& other) noexcept
		: m_Buffer(other.m_Buffer), 
		m_BufferEnd(other.m_BufferEnd),
		m_Size(other.m_Size)
	{
		other.m_Buffer = nullptr;
		other.m_BufferEnd = nullptr;
		other.m_Size = 0;
	}

	CommandStream& CommandStream::operator=(CommandStream&& other) noexcept
	{
		m_Buffer = other.m_Buffer;
		m_BufferEnd = other.m_BufferEnd;
		m_Size = other.m_Size;
		other.m_Buffer = nullptr;
		other.m_BufferEnd = nullptr;
		other.m_Size = 0;

		return *this;
	}

	void CommandStream::Flush()
	{
		void* elem = m_Buffer;

		while(elem != m_BufferEnd)
		{
			CommandFn* cmd = (CommandFn*)elem;
			elem = (void*)((uint64)elem + sizeof(CommandFn));

			void* func = elem;
			uint16 funcSize = (*cmd)(func);

			elem = (void*)((uint64)elem + funcSize);
		}

		m_BufferEnd = m_Buffer;
	}
}
