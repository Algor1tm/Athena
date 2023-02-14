#pragma once

#include "Athena/Core/Core.h"

#include <string_view>


namespace Athena
{
	struct SystemInfo
	{
		String CPUBrandString;
		uint32 NumberOfProcessorCores = 0;
		uint32 NumberOfLogicalProcessors = 0;

		uint64 TotalPhysicalMemoryKB = 0;
	};

	class ATHENA_API Platform
	{
	public:
		static void GetSystemInfo(SystemInfo* info);

		// returns time in milliseconds
		static double GetHighPrecisionTime();
	};

	class ATHENA_API FileDialogs
	{
	public:
		static FilePath OpenFile(std::string_view filter);
		static FilePath SaveFile(std::string_view filter);
	};
}
