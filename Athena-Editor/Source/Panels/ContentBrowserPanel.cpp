#include "ContentBrowserPanel.h"

#include "Athena/Renderer/Texture.h"

#include "Athena/UI/Widgets.h"

#include "EditorResources.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>

#include <string_view>


namespace Athena
{
	ContentBrowserPanel::ContentBrowserPanel(std::string_view name)
		: Panel(name)
	{
		m_LastDirectory = m_CurrentDirectory = m_AssetDirectory;
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		if (ImGui::Begin("Content Browser"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 5.f, 10.f });
			ImGui::PushStyleColor(ImGuiCol_Button, UI::GetDarkColor());

			if (ImGui::ImageButton(EditorResources::GetIcon("ContentBrowser_Undo")->GetRendererID(), m_UndoButtonSize, {0, 1}, {1, 0}))
			{
				if (m_CurrentDirectory != m_AssetDirectory)
				{
					m_LastDirectory = m_CurrentDirectory;
					m_CurrentDirectory = m_CurrentDirectory.parent_path();
				}
			}

			ImGui::SameLine();

			if (ImGui::ImageButton(EditorResources::GetIcon("ContentBrowser_Redo")->GetRendererID(), m_UndoButtonSize, { 0, 1 }, { 1, 0 }))
			{
				if (m_CurrentDirectory != m_LastDirectory)
				{
					m_CurrentDirectory = m_LastDirectory;
				}
			}

			ImGui::SameLine();

			ImVec2 regionAvail = ImGui::GetContentRegionAvail();

			ImGui::PushItemWidth(regionAvail.x * 0.15f);
			String searchString;
			UI::TextInputWithHint("Search...", searchString); // TODO: make this

			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			ImGui::SameLine();

			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + regionAvail.x * 0.01f);
			ImGui::Text(m_CurrentDirectory.string().c_str());


			ImGui::Separator();

			float cellSize = m_Padding + m_ItemSize.x;
			float panelWidth = regionAvail.x;

			ImGui::Columns(int(panelWidth / cellSize), 0, false);

			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
			for (const auto& dirEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
			{
				const auto& relativePath = dirEntry.path();
				const auto& filename = dirEntry.path().filename().string();

				ImGui::PushID(filename.data());
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0, 0, 0, 0 });
				Ref<Texture2D> icon = dirEntry.is_directory() ? EditorResources::GetIcon("ContentBrowser_Folder") : EditorResources::GetIcon("ContentBrowser_File");
				ImGui::ImageButton(icon->GetRendererID(), m_ItemSize, {0, 1}, {1, 0});

				if (dirEntry.is_directory() && ImGui::IsItemClicked())
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
			ImGui::PopStyleVar();

			ImGui::EndColumns();

		}

		ImGui::End();
	}
}
