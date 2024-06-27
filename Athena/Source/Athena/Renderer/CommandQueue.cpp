#include "CommandQueue.h"

#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	CommandQueue::CommandQueue(uint64 sizeInBytes)
	{
		m_Buffer = new byte[sizeInBytes];
		m_BufferEnd = m_Buffer;
	}

	CommandQueue::~CommandQueue()
	{
		delete[] m_Buffer;
	}

	CommandQueue::CommandQueue(CommandQueue&& other) noexcept
		: m_Buffer(other.m_Buffer), 
		  m_BufferEnd(other.m_BufferEnd)
	{
		other.m_Buffer = nullptr;
		other.m_BufferEnd = nullptr;
	}

	CommandQueue& CommandQueue::operator=(CommandQueue&& other) noexcept
	{
		m_Buffer = other.m_Buffer;
		m_BufferEnd = other.m_BufferEnd;
		other.m_Buffer = nullptr;
		other.m_BufferEnd = nullptr;

		return *this;
	}

	void CommandQueue::Flush()
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
