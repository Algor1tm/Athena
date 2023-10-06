#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API SwapChain
	{
	public:
		static Ref<SwapChain> Create(void* windowHandle);
		virtual ~SwapChain() = default;

		virtual void AcquireImage() = 0;
		virtual void Present() = 0;

		virtual void* GetCurrentImageHandle() = 0;
	};
}
