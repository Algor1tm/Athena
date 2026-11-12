#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	inline uint64 AlignUp(uint64 value, uint64 alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	class ATHENA_API CommandStream
	{
	private:
		static constexpr size_t CommandAlignment = 16;

		using CommandExecutor = uint64 (*)(void*);
		using CommandRelocator = uint64(*)(void*, void*);

		template <typename FuncT>
		struct alignas(CommandAlignment) CommandWrapper
		{
			CommandExecutor Executor;
			CommandRelocator Relocator;
			FuncT Function;
		};
		
	public:
		CommandStream() = default;
		CommandStream(uint64 sizeInBytes);
		~CommandStream();

		CommandStream(CommandStream&& other) noexcept;
		CommandStream& operator=(CommandStream&& other) noexcept;

		NON_COPYABLE(CommandStream);

		void Flush();
		void Resize(uint64 newSize);

		template <typename FuncT>
		void Enqueue(FuncT&& func)
		{
			using StoredType = std::decay_t<FuncT>;
			using CommandType = CommandWrapper<StoredType>;

			static_assert(alignof(CommandType) == CommandAlignment, "Command alignment exceeds maximum supported alignment");
			static_assert(std::is_move_constructible_v<StoredType>, "Command must be nothrow move constructible");

			if (m_BufferOffset + sizeof(CommandType) > m_Size)
			{
				constexpr uint32 growthFactor = 2;
				Resize((m_Size + sizeof(CommandType)) * growthFactor);
			}

			CommandExecutor executor = [](void* ptr) -> uint64
			{
				CommandType* command = (CommandType*)ptr;
				command->Function();

				command->~CommandType();

				return sizeof(CommandType);
			};

			CommandRelocator relocator = [](void* src, void* dst) -> uint64
			{
				CommandType* source = (CommandType*)src;

				new(dst) CommandType{ source->Executor,source->Relocator, std::move(source->Function) };
				source->~CommandType();

				return sizeof(CommandType);
			};

			void* ptr = (byte*)m_Buffer + m_BufferOffset;

			new(ptr) CommandType{ executor, relocator, std::forward<FuncT>(func) };
			m_BufferOffset += sizeof(CommandType);
		}

	private:
		void* m_Buffer = nullptr;
		uint64 m_BufferOffset = 0;
		uint64 m_Size = 0;
	};
}
