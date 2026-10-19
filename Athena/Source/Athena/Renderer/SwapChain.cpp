#include "SwapChain.h"
#include "Athena/Renderer/RenderThread.h"
#include "Athena/Renderer/RHI/RHI.h"


namespace Athena
{
	Ref<SwapChain> SwapChain::Create(void* windowHandle, bool vsync)
	{
		Ref<SwapChain> swapChain = RHI::Get()->CreateSwapChain(windowHandle, vsync);

		ENQUEUE_RENDER_COMMAND([swapChain]() 
			{
				swapChain->Init();
			});

		return swapChain;
	}
}
