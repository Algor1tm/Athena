#pragma once

#include "Athena/Core/Core.h"

#include <string_view>


namespace Athena
{
	class ATHENA_API FileDialogs
	{
	public:
		static String OpenFile(std::string_view filter);
		static String SaveFile(std::string_view filter);
	};
}
