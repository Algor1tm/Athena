#pragma once

#include "Athena/Core/Core.h"

#include <string_view>


namespace Athena
{
	struct CPUCapabilities
	{
		String Name;
		uint32 Cores = 0;
		uint32 LogicalProcessors = 0;
		uint64 RAM = 0;	// Kb
	};

	class ATHENA_API Platform
	{
	public:
		static void Init();
		static const CPUCapabilities& GetCPUCapabilities();

		// In milliseconds
		static double GetHighPrecisionTime();

		// In bytes
		static uint64 GetMemoryUsage();
	};

	class ATHENA_API FileDialogs
	{
	public:
		static FilePath OpenFile(std::string_view filter);
		static FilePath SaveFile(std::string_view filter);
	};
}
