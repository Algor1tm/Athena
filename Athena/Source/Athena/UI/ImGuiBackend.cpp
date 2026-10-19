#include "ImGuiBackend.h"

#include "Athena/Renderer/RenderThread.h"
#include "Athena/Renderer/RHI/RHI.h"


namespace Athena
{
	Ref<ImGuiBackend> ImGuiBackend::Create(void* windowHandle)
	{
		Ref<ImGuiBackend> backend = RHI::Get()->CreateImGuiBackend();

		ENQUEUE_RENDER_COMMAND([backend, windowHandle]() 
			{
				backend->Init(windowHandle);
			});

		return backend;
	}
}
