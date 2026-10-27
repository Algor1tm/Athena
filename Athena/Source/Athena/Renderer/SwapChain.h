#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API SwapChain : public RefCounted
	{
	public:
		static Ref<SwapChain> Create(void* windowHandle, uint32 width, uint32 height, bool vsync = false);
		virtual ~SwapChain() = default;

		virtual void Init() = 0;
		virtual void OnWindowResized(uint32 width, uint32 height) = 0;
		virtual void SetVSync(bool enabled) = 0;
	};
}
