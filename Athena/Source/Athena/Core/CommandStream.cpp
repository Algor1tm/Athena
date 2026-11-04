#include "CommandStream.h"
#include "Athena/Math/Common.h"


namespace Athena
{
	CommandStream::CommandStream(uint64 sizeInBytes)
	{
		m_Size = AlignUp(sizeInBytes, CommandAlignment);
		m_Buffer = new byte[m_Size];
		m_BufferOffset = 0;
	}

	CommandStream::~CommandStream()
	{
		delete[] m_Buffer;
	}

	CommandStream::CommandStream(CommandStream&& other) noexcept
		: m_Buffer(other.m_Buffer), 
		m_BufferOffset(other.m_BufferOffset),
		m_Size(other.m_Size)
	{
		other.m_Buffer = nullptr;
		other.m_BufferOffset = 0;
		other.m_Size = 0;
	}

	CommandStream& CommandStream::operator=(CommandStream&& other) noexcept
	{
		if (this == &other)
			return *this;

		Flush();
		delete[] m_Buffer;

		m_Buffer = other.m_Buffer;
		m_BufferOffset = other.m_BufferOffset;
		m_Size = other.m_Size;
		other.m_Buffer = nullptr;
		other.m_BufferOffset = 0;
		other.m_Size = 0;

		return *this;
	}

	void CommandStream::Flush()
	{
		void* commandIterator = m_Buffer;
		void* bufferEnd = (byte*)m_Buffer + m_BufferOffset;

		while(commandIterator != bufferEnd)
		{
			CommandExecutor* executor = (CommandExecutor*)commandIterator;
			uint64 commandSize = (*executor)(commandIterator);

			commandIterator = (byte*)commandIterator + commandSize;
		}

		m_BufferOffset = 0;
	}

	void CommandStream::Resize(uint64 newSize)
	{
		newSize = AlignUp(Math::Max(newSize, m_BufferOffset), CommandAlignment);

		byte* newBuffer = new byte[newSize];

		byte* src = (byte*)m_Buffer;
		byte* dst = newBuffer;

		byte* bufferEnd = src + m_BufferOffset;

		while (src != bufferEnd)
		{
			auto* relocator = reinterpret_cast<CommandRelocator*>(src + sizeof(CommandExecutor));

			uint64 commandSize = (*relocator)(src, dst);

			src += commandSize;
			dst += commandSize;
		}

		delete[](byte*)m_Buffer;

		m_Buffer = newBuffer;
		m_Size = newSize;
	}
}
