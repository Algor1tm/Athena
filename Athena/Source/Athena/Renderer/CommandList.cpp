#include "CommandList.h"
#include "Athena/Renderer/RHI/RHI.h"


namespace Athena
{
	Ref<CommandList> CommandList::Create()
	{
		return RHI::Get()->CreateCommandList();
	}
}