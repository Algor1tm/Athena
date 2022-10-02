#include "ContentBrowserPanel.h"

#include "UI/Widgets.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>

#include <string_view>


namespace Athena
{
	ContentBrowserPanel::ContentBrowserPanel(std::string_view name)
		: Panel(name)
	{
		m_FolderIcon = Texture2D::Create("Resources/Icons/Editor/ContentBrowser/FolderIcon.png");
		m_FileIcon = Texture2D::Create("Resources/Icons/Editor/ContentBrowser/FileIcon.png");
		m_BackButtonIcon = Texture2D::Create("Resources/Icons/Editor/ContentBrowser/BackButtonIcon.png");

		m_CurrentDirectory = m_AssetDirectory;
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");

		if (m_CurrentDirectory != m_AssetDirectory)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0, 0, 0, 0 });
			if (ImGui::ImageButton(m_BackButtonIcon->GetRendererID(), { m_BackButtonSize.x, m_BackButtonSize.y }, {0, 1}, {1, 0}))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
			ImGui::PopStyleColor();
		}

		float cellSize = m_Padding + m_ItemSize.x;
		float panelWidth = ImGui::GetContentRegionAvail().x;

		ImGui::Columns(int(panelWidth / cellSize), 0, false);

		for (const auto& dirEntry: std::filesystem::directory_iterator(m_CurrentDirectory))
		{
			const auto& relativePath = dirEntry.path();
			const auto& filename = dirEntry.path().filename().string();

			ImGui::PushID(filename.data());
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0, 0, 0, 0 });
			ImGui::ImageButton(dirEntry.is_directory() ? m_FolderIcon->GetRendererID() : m_FileIcon->GetRendererID(), { m_ItemSize.x, m_ItemSize.y }, { 0, 1 }, { 1, 0 });

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
