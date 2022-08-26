#include "ContentBrowserPanel.h"

#include <ImGui/imgui.h>

#include <string_view>


namespace Athena
{
	ContentBrowserPanel::ContentBrowserPanel()
	{
		m_CurrentDirectory = m_AssetDirectory;
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");

		if (m_CurrentDirectory != m_AssetDirectory)
		{
			if (ImGui::Button("<-"))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
		}

		for (const auto& dir_entry: std::filesystem::directory_iterator(m_CurrentDirectory))
		{
			std::filesystem::path filename = dir_entry.path().filename();
			if (dir_entry.is_directory())
			{
				if (ImGui::Button(filename.string().data()))
				{
					m_CurrentDirectory /= filename;
				}
			}
			else
			{
				ImGui::Text(filename.string().data());
			}
		}

		ImGui::End();
	}
}
