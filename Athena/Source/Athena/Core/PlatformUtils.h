#pragma once

#include "Athena/Core/Core.h"

#include <string_view>


namespace Athena
{
	class ATHENA_API FileDialogs
	{
	public:
		static Filepath OpenFile(std::string_view filter);
		static Filepath SaveFile(std::string_view filter);
	};
}
