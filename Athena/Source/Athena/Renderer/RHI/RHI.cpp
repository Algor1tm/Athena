#include "RHI.h"

#include "Athena/Core/ConsoleManager.h"

namespace Athena
{
	static AutoCVar<int32> CVarMaxFramesInFlight(
		"r.MaxFramesInFlight",
		3,
		"Sets max frames in flight for RHI backends that support it, can be overriden by gpu capabilities (default - 3)."
	);
}
