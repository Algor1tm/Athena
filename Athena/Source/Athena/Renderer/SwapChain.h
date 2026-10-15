#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API SwapChain : public RefCounted
	{
	public:
		static Ref<SwapChain> Create(void* windowHandle, bool vsync = false);

		virtual void Init() = 0;
		virtual void Destroy() = 0;

		virtual ~SwapChain() = default;

		virtual void OnWindowResized() = 0;
		virtual void SetVSync(bool enabled) = 0;
	};
}
