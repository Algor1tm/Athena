#include "CommandList.h"
#include "Athena/Renderer/RHI/RHI.h"


namespace Athena
{
	Ref<CommandList> CommandList::Create()
	{
		return RHI::Get()->CreateCommandList();
	}

#if ATN_ENABLE_TRACY_GPU
	TracyGPUZoneScoped::TracyGPUZoneScoped(const Ref<CommandList>& cmdList, uint32 line, std::string_view fileName, std::string_view functionName, const String& name)
	{
		TracyZone = RHI::Get()->CreateTracyZone(cmdList, line, fileName, functionName, name);
	}

	TracyGPUZoneScoped::~TracyGPUZoneScoped()
	{
		TracyZone.Release();
	}
#endif
}
