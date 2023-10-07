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

		return result;
	}

	std::vector<byte> FileSystem::ReadFileBinary(const FilePath& path)
	{
		std::vector<byte> result;

		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (in.is_open())
		{
			in.seekg(0, std::ios::end);
			auto size = in.tellg();
			in.seekg(0, std::ios::beg);

			result.resize(size);
			in.read((char*)result.data(), size);
		}
		
		return result;
	}

	bool FileSystem::WriteFile(const FilePath& path, const char* bytes, uint64 size)
	{
		std::ofstream out(path, std::ios::out | std::ios::binary);
		if (out.is_open())
		{
			out.write(bytes, size);
			out.flush();
			out.close();
			return true;
		}

		return false;
	}

	FilePath FileSystem::GetWorkingDirectory()
	{
		return std::filesystem::current_path();
	}

	void FileSystem::SetWorkingDirectory(const FilePath& path)
	{
		std::filesystem::current_path(path);
	}

	void FileSystem::CreateDirectory(const FilePath& path)
	{
		std::filesystem::create_directories(path);
	}

	bool FileSystem::Exists(const FilePath& path)
	{
		return std::filesystem::exists(path);
	}
}
