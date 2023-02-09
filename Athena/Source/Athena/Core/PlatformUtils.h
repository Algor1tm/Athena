#pragma once

#include "Athena/Core/Core.h"

#include <string_view>


namespace Athena
{
	class ATHENA_API FileDialogs
	{
	public:
		static FilePath OpenFile(std::string_view filter);
		static FilePath SaveFile(std::string_view filter);
	};
}
