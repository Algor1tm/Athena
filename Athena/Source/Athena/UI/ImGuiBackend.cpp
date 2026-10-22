#include "ImGuiBackend.h"

#include "Athena/Renderer/RenderThread.h"
#include "Athena/Renderer/RHI/RHI.h"


namespace Athena
{
	Ref<ImGuiBackend> ImGuiBackend::Create()
	{
		Ref<ImGuiBackend> backend = RHI::Get()->CreateImGuiBackend();
		return backend;
	}
}
