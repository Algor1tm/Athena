#include "CommandContext.h"
#include "Athena/Renderer/RHI/RHI.h"


namespace Athena
{
	void CommandContext::Transition(Texture* texture, ResourceState dst)
	{
		Transition(texture, texture->GetFullSubresourceRange(), dst);
	}

	void CommandContext::TransitionOwnership(Texture* texture, CommandContextType dstContext, ResourceState dst)
	{
		TransitionOwnership(texture, texture->GetFullSubresourceRange(), dstContext, dst);
	}
}
