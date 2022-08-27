#include "ContentBrowserPanel.h"
#include "UI/Controllers.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>

#include <string_view>


namespace Athena
{
	ContentBrowserPanel::ContentBrowserPanel()
	{
		m_CurrentDirectory = m_AssetDirectory;

		m_FolderIcon = Texture2D::Create("Resources/Icons/FolderIcon.png");
		m_FolderIconID = m_FolderIcon->GetRendererID();

		m_FileIcon = Texture2D::Create("Resources/Icons/FileIcon.png");
		m_FileIconID = m_FileIcon->GetRendererID();

		m_BackButtonIcon = Texture2D::Create("Resources/Icons/BackButtonIcon.png");
		m_BackButtonIconID = m_BackButtonIcon->GetRendererID();
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");

		if (m_CurrentDirectory != m_AssetDirectory)
		{
			if (ImGui::ImageButton(reinterpret_cast<void*>((uint64)m_BackButtonIconID), m_BackButtonSize, { 0, 1 }, { 1, 0 }))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
		}

		float cellSize = m_Padding + m_ItemSize.x;
		float panelWidth = ImGui::GetContentRegionAvail().x;

		ImGui::Columns(int(panelWidth / cellSize), 0, false);

		for (const auto& dir_entry: std::filesystem::directory_iterator(m_CurrentDirectory))
		{
			const auto& filename = dir_entry.path().filename().string();

			if (dir_entry.is_directory())
			{
				ImGui::ImageButton(reinterpret_cast<void*>((uint64)m_FolderIconID), m_ItemSize, { 0, 1 }, { 1, 0 });
				if(ImGui::IsItemClicked())
				{
					m_CurrentDirectory /= filename;
				}
			}
			else
			{
				ImGui::ImageButton(reinterpret_cast<void*>((uint64)m_FileIconID), m_ItemSize, { 0, 1 }, { 1, 0 });
			}

			ImGui::TextWrapped(filename.data());

			ImGui::NextColumn();
		}

		ImGui::EndColumns();

		ImGui::End();
	}
}
