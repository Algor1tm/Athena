#include "FileSystem.h"


namespace Athena
{
	String FileSystem::ReadFile(const Filepath& path)
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
			ATN_CORE_ERROR("Could not open file: '{0}'", path);
		}

		return result;
	}
}
