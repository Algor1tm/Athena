#include "SwapChain.h"
#include "Athena/Renderer/RHI/RHI.h"


namespace Athena
{
	Ref<SwapChain> SwapChain::Create(void* windowHandle, bool vsync)
	{
		return RHI::Get()->CreateSwapChain(windowHandle, vsync);
	}
}
