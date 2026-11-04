#include "CommandContext.h"
#include "Athena/Renderer/RHI/RHI.h"


namespace Athena
{
	void CommandContext::Transition(Texture* texture, PipelineAccess src, PipelineAccess dst)
	{
		Transition(texture, texture->GetFullRange(), src, dst);
	}

	void CommandContext::TransitionOwnership(Texture* texture, PipelineAccess src, CommandContextType dstContext, PipelineAccess dst)
	{
		TransitionOwnership(texture, texture->GetFullRange(), src, dstContext, dst);
	}

#if ATN_ENABLE_TRACY_GPU
	TracyGPUZoneScoped::TracyGPUZoneScoped(const Ref<CommandContext>& cmdContext, uint32 line, std::string_view fileName, std::string_view functionName, const String& name)
	{
		if(cmdContext->HasTracyContext())
			TracyZone = RHI::Get()->CreateTracyZone(cmdContext, line, fileName, functionName, name);
	}

	TracyGPUZoneScoped::~TracyGPUZoneScoped()
	{
		TracyZone.Release();
	}
#endif
}
