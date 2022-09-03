#pragma once

#include "Athena/Core/Core.h"

#include <string_view>


namespace Athena
{
	class ATHENA_API FileDialogs
	{
	public:
		static String OpenFile(const char* filter);
		static String SaveFile(const char* filter);

	private:
	};
}
