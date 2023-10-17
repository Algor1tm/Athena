#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	enum class CommandBufferUsage
	{
		PRESENT,
		IMMEDIATE
	};

	class ATHENA_API CommandBuffer
	{
	public:
		static Ref<CommandBuffer> Create(CommandBufferUsage usage);
		virtual ~CommandBuffer() = default;

		virtual void Begin() = 0;
		virtual void End() = 0;

		virtual void Flush() = 0;
	};
}
