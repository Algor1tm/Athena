#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API CommandQueue
	{
	private:
		using CommandFn = uint16 (*)(void*);

	public:
		CommandQueue() = default;
		CommandQueue(uint64 sizeInBytes);
		~CommandQueue();

		CommandQueue(CommandQueue&& other) noexcept;
		CommandQueue& operator=(CommandQueue&& other)  noexcept;

		CommandQueue(const CommandQueue&) = delete;
		CommandQueue& operator=(const CommandQueue& other) = delete;

		void Flush();

		template <typename FuncT>
		void Submit(FuncT&& func)
		{
			CommandFn commandFn = [](void* func) -> uint16
			{
				FuncT* funcT = (FuncT*)func;
				(*funcT)();
				funcT->~FuncT();

				return sizeof(FuncT);
			};

			// Element size = sizeof(CommandFn) + sizeof(FuncT)

			new(m_BufferEnd) CommandFn(commandFn);
			m_BufferEnd = (void*)((uint64)m_BufferEnd + sizeof(CommandFn));

			new(m_BufferEnd) FuncT(func);
			m_BufferEnd = (void*)((uint64)m_BufferEnd + sizeof(FuncT));
		}

	private:
		void* m_Buffer;
		void* m_BufferEnd;
	};
}
