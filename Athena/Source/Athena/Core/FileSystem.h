#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API FileSystem
	{
	public:
		static String ReadFile(const Filepath& path);
	};
}
