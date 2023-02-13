#include "FileSystem.h"

#include <filesystem>


namespace Athena
{
	String FileSystem::ReadFile(const FilePath& path)
	{
		String result;
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(result.data(), result.size());
			in.close();
		}
		else
		{
			ATN_CORE_ERROR("FileSystem::ReadFile: Could not open file: '{0}'", path);
		}

		return result;
	}

	FilePath FileSystem::GetWorkingDirectory()
	{
		return std::filesystem::current_path();
	}

	void FileSystem::SetWorkingDirectory(const FilePath& path)
	{
		std::filesystem::current_path(path);
	}

	bool FileSystem::Exists(const FilePath& path)
	{
		return std::filesystem::exists(path);
	}
}
