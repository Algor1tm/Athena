#include "ContentBrowserPanel.h"
#include "UI/Controllers.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>

#include <string_view>


namespace Athena
{
	ContentBrowserPanel::ContentBrowserPanel()
		: m_FolderIcon("Resources/Icons/FolderIcon.png"), m_FileIcon("Resources/Icons/FileIcon.png"), 
		m_BackButtonIcon("Resources/Icons/BackButtonIcon.png")
	{
		m_CurrentDirectory = m_AssetDirectory;
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");

		if (m_CurrentDirectory != m_AssetDirectory)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0, 0, 0, 0 });
			if (ImGui::ImageButton(m_BackButtonIcon.GetRendererIDvoid(), m_BackButtonSize, {0, 1}, {1, 0}))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
			ImGui::PopStyleColor();
		}

		float cellSize = m_Padding + m_ItemSize.x;
		float panelWidth = ImGui::GetContentRegionAvail().x;

		ImGui::Columns(int(panelWidth / cellSize), 0, false);

		void* iconID;
		for (const auto& dirEntry: std::filesystem::directory_iterator(m_CurrentDirectory))
		{
			const auto& relativePath = dirEntry.path();
			const auto& filename = dirEntry.path().filename().string();
			iconID = dirEntry.is_directory() ? m_FolderIcon.GetRendererIDvoid() : m_FileIcon.GetRendererIDvoid();

			ImGui::PushID(filename.data());
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0, 0, 0, 0 });
			ImGui::ImageButton(iconID, m_ItemSize, { 0, 1 }, { 1, 0 });

			if(dirEntry.is_directory() && ImGui::IsItemClicked())
			{
				m_CurrentDirectory /= filename;
			}
			else
			{
				if (ImGui::BeginDragDropSource())
				{
					const auto& path = relativePath.string();

					ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", path.data(), strlen(path.data()) + 1, ImGuiCond_Once);
					ImGui::EndDragDropSource();
				}
			}
			ImGui::PopStyleColor();
			ImGui::PopID();

			ImGui::TextWrapped(filename.data());
			ImGui::NextColumn();
		}

		ImGui::EndColumns();

		ImGui::End();
	}
}
