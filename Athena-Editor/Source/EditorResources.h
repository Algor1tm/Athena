#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"

#include <unordered_map>


namespace Athena
{
	class EditorResources
	{
	public:
		static void Init(const FilePath& path);

		static const FilePath& GetPath();
		static Ref<Texture2D> GetIcon(std::string_view name);


	private:
		static FilePath m_Path;
		static std::unordered_map<std::string_view, Ref<Texture2D>> m_Icons;
	};
}