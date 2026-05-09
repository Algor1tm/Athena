#include "AssetManagerPanel.h"
#include "Panels/PanelManager.h"
#include "Athena/Project/Project.h"
#include "Athena/UI/UI.h"
#include "Athena/UI/Theme.h"
#include "Athena/Utils/StringUtils.h"


namespace Athena
{
	AssetManagerPanel::AssetManagerPanel(const Ref<EditorContext>& context)
		: Panel(ASSET_MANAGER_PANEL_ID, context)
	{

	}

	void AssetManagerPanel::OnImGuiRender()
	{
		ImGui::Begin("AssetManager");

		const auto& registry = Project::GetEditorAssetManager()->GetAssetRegistry().GetRegistry();

		if (UI::TreeNode("AssetRegistry", true))
		{
			UI::TextInput("AssetRegistrySearch", m_SearchString);

			registry.for_each([this](const std::pair<AssetHandle, AssetMetadata>& element) 
			{
				const auto& [handle, metadata] = element;

				if (Filter(handle, metadata))
				{
					UI::BeginPropertyTable();

					String buffer = std::to_string(handle);
					UI::PropertyRow("AssetHandle", ImGui::GetFrameHeight());

					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
					UI::TextInput("AssetHandleTextInput", buffer, ImGuiInputTextFlags_ReadOnly);
					ImGui::PopStyleColor();


					buffer = metadata.FilePath.string();
					UI::PropertyRow("FilePath", ImGui::GetFrameHeight());

					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
					UI::TextInput("FilePathTextInput", buffer, ImGuiInputTextFlags_ReadOnly);
					ImGui::PopStyleColor();


					buffer = AssetManager::AssetTypeToString(metadata.Type);
					UI::PropertyRow("Type", ImGui::GetFrameHeight());

					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
					UI::TextInput("TypeTextInput", buffer, ImGuiInputTextFlags_ReadOnly);
					ImGui::PopStyleColor();

					UI::EndPropertyTable();
					UI::ShiftCursorY(5.f);
				}
			});

			UI::TreePop();
		}

		ImGui::End();
	}

	bool AssetManagerPanel::Filter(AssetHandle handle, const AssetMetadata& metadata)
	{
		if (m_SearchString.empty())
			return true;

		String searchString = Utils::ToLower(m_SearchString);

		AssetHandle handleSearch = std::atoll(searchString.c_str());
		if (handleSearch == handle)
			return true;

		String typeSearch = Utils::ToLower(String(AssetManager::AssetTypeToString(metadata.Type)));
		if (typeSearch.find(searchString) != std::string::npos)
			return true;

		String filePathSearch = Utils::ToLower(metadata.FilePath.string());
		if (filePathSearch.find(searchString) != std::string::npos)
			return true;

		return false;
	}
}
