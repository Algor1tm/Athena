#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API ConstantBuffer
	{
	public:
		virtual ~ConstantBuffer() = default;

		static Ref<ConstantBuffer> Create(uint32 size, uint32 binding);

		virtual void SetData(const void* data, uint32 size, uint32 offset = 0) = 0;
	};
}
