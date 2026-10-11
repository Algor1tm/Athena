#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API CommandStream
	{
	private:
		using CommandFn = uint16 (*)(void*);

	public:
		CommandStream() = default;
		CommandStream(uint64 sizeInBytes);
		~CommandStream();

		CommandStream(CommandStream&& other) noexcept;
		CommandStream& operator=(CommandStream&& other) noexcept;

		CommandStream(const CommandStream& other) = delete;
		CommandStream& operator=(const CommandStream& other) = delete;

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

			check(((byte*)m_BufferEnd + sizeof(CommandFn)  + sizeof(FuncT) - (byte*)m_Buffer) <= m_Size && m_Size != 0, "CommandStream buffer overflow, incease size!");

			new(m_BufferEnd) CommandFn(commandFn);
			m_BufferEnd = (void*)((uint64)m_BufferEnd + sizeof(CommandFn));

			new(m_BufferEnd) FuncT(std::forward<FuncT>(func));
			m_BufferEnd = (void*)((uint64)m_BufferEnd + sizeof(FuncT));
		}

	private:
		void* m_Buffer = nullptr;
		void* m_BufferEnd = nullptr;
		uint64 m_Size = 0;
	};
}
