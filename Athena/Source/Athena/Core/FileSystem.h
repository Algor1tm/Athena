#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Buffer.h"

#ifdef CreateDirectory
	#undef CreateDirectory
#endif


namespace Athena
{
	class ATHENA_API FileSystem
	{
	public:
		static String ReadFile(const FilePath& path);
		static Buffer ReadFileBinary(const FilePath& path);

		static bool WriteFile(const FilePath& path, const char* bytes, uint64 size);
		static bool Remove(const FilePath& path);
		static void CreateDirectory(const FilePath& path);
		static bool Copy(const FilePath& from, const FilePath& to);

		static FilePath GetWorkingDirectory();
		static void SetWorkingDirectory(const FilePath& path);

		static bool Exists(const FilePath& path);
	};
}
