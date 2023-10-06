#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API CommandBuffer
	{
	public:
		static Ref<CommandBuffer> Create();
		virtual ~CommandBuffer() = default;

		virtual void Begin() = 0;
		virtual void End() = 0;

		// Temporary function
		virtual void* GetCommandBuffer() = 0;
	};
}
