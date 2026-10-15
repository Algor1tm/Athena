#pragma once

#include "Athena/Core/Core.h"

namespace Athena
{
	class ATHENA_API CommandList: public RefCounted
	{
	public:
		static Ref<CommandList> Create();
		virtual ~CommandList() = default;

		virtual void Submit(bool wait = false) = 0;
	};
}
