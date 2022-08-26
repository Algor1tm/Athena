#pragma once

#include "Athena/Core/Core.h"

#include <filesystem>


namespace Athena
{
	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender();

	private:
		std::filesystem::path m_CurrentDirectory;
		std::string_view m_AssetDirectory = "assets";
	};
}
