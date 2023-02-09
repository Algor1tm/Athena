#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API FileSystem
	{
	public:
		static String ReadFile(const FilePath& path);

		static FilePath GetWorkingDirectory();
		static void SetWorkingDirectory(const FilePath& path);

		static bool Exists(const FilePath& path);
	};
}
