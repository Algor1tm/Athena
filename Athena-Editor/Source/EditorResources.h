#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Image.h"

#include <unordered_map>


namespace Athena
{
	class EditorResources
	{
	public:
		static void Init(const FilePath& path);
		static void Shutdown();

		static const FilePath& GetPath();
		static Ref<Image> GetIcon(std::string_view name);


	private:
		static FilePath m_Path;
		static std::unordered_map<std::string_view, Ref<Image>> m_Icons;
	};
}