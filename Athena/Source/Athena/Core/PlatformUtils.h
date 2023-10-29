#pragma once

#include "Athena/Core/Core.h"

#include <string_view>


namespace Athena
{
	struct CPUCapabilities
	{
		String Name;
		uint32 NumberOfProcessorCores = 0;
		uint32 NumberOfLogicalProcessors = 0;
		uint64 TotalPhysicalMemoryKB = 0;
	};

	class ATHENA_API Platform
	{
	public:
		static void Init();
		static const CPUCapabilities& GetCPUCapabilities();

		// returns time in milliseconds
		static double GetHighPrecisionTime();
		static uint64 GetMemoryUsage();
	};

	class ATHENA_API FileDialogs
	{
	public:
		static FilePath OpenFile(std::string_view filter);
		static FilePath SaveFile(std::string_view filter);
	};
}
