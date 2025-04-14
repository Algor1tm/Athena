#include "ContentBrowserPanel.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Core/PlatformUtils.h"
#include "Athena/Project/Project.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/TextureGenerator.h"
#include "Athena/UI/UI.h"
#include "Athena/UI/Theme.h"
#include "Athena/Utils/StringUtils.h"

#include "Panels/PanelManager.h"
#include "EditorResources.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>

#include <string_view>
#include <queue>


namespace Athena
{
	CBItem::CBItem(ContentBrowserPanel* panel, const FilePath& path)
	{
		m_FilePath = path.string();
		m_FileName = path.filename().string();
		m_ContentBrowserPanel = panel;
	}

	void CBItem::TrackMouseState(ImVec2 itemSize)
	{
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();

		ImGui::InvisibleButton("ContentItem", itemSize);

		bool isItemHovered = ImGui::IsItemHovered();
		bool isItemClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && isItemHovered;
		bool isItemDoubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && isItemHovered;
		bool isItemRightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right) && isItemHovered;

		m_State.SetIf(isItemHovered, CBItemStateFlag_Hovered);
		m_State.SetIf(isItemDoubleClicked, CBItemStateFlag_Active);

		if (isItemClicked || isItemRightClicked)
		{
			m_ContentBrowserPanel->DeselectItem();
			m_State.Set(CBItemStateFlag_Selected);
		}

		if (isItemRightClicked)
		{
			m_State.Set(CBItemStateFlag_ActivePopup);

			ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Always, ImVec2(0.f, 1.0f));
			ImGui::OpenPopup("CBItemPopup");
		}

		ImGui::SetCursorScreenPos(cursorPos);
	}

	bool CBItem::LexCompare(const Ref<CBItem>& item) const
	{
		bool isLeftFolder = IsFolder();
		bool isRightFolder = item->IsFolder();

		if (isLeftFolder && !isRightFolder)
			return true;

		if (!isLeftFolder && isRightFolder)
			return false;

		const String& leftName = GetFileName();
		const String& rightName = item->GetFileName();

		return std::lexicographical_compare(leftName.begin(), leftName.end(), rightName.begin(), rightName.end());
	}


	CBFolder::CBFolder(ContentBrowserPanel* panel, const FilePath& path)
		: CBItem(panel, path)
	{
		for (const auto& dirEntry : std::filesystem::directory_iterator(GetFilePath()))
		{
			bool isFolder = dirEntry.is_directory();
			const FilePath& dirPath = dirEntry.path();

			Ref<CBItem> item;
			if (isFolder)
			{
				item = Ref<CBFolder>::Create(panel, dirPath);
			}
			else
			{
				FilePath relativePath = AssetManager::GetAssetRelativePath(dirPath);
				AssetHandle handle = Project::GetEditorAssetManager()->GetAssetHandleFromFilePath(relativePath);
				if (Project::GetEditorAssetManager()->IsAssetHandleValid(handle))
					item = Ref<CBAssetItem>::Create(panel, dirPath);
			}

			if (item)
				m_ChildrenItems.push_back(item);
		}

		std::sort(m_ChildrenItems.begin(), m_ChildrenItems.end(), [this](const Ref<CBItem>& left, const Ref<CBItem>& right)
			{
				return left->LexCompare(right);
			});
	}

	void CBFolder::OnImGuiRender(ImVec2 itemSize)
	{
		const String& fileName = GetFileName();

		TrackMouseState(itemSize);
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// Borders
		{
			bool isSelected = m_State.IsSet(CBItemStateFlag_Selected);
			bool isHovered = m_State.IsSet(CBItemStateFlag_Hovered);

			const ImU32 borderColor = isSelected ? UI::GetTheme().Accent : UI::GetTheme().TabActive;

			if (isSelected || isHovered)
			{
				drawList->AddRect(ImVec2(cursorPos.x, cursorPos.y),
					ImVec2(cursorPos.x + itemSize.x, cursorPos.y + itemSize.y),
					borderColor, 3.0f);
			}
		}

		// Image
		{
			ImVec2 iconSize = { itemSize.x, itemSize.y * IMAGE_TO_ITEM_RATIO };
			ImVec2 contentPos = cursorPos;

			drawList->AddImage(UI::GetTextureID(EditorResources::GetIcon("ContentBrowser_Folder")),
				contentPos,
				ImVec2(contentPos.x + iconSize.x, contentPos.y + iconSize.y));

			contentPos.y += iconSize.y;
			ImGui::SetCursorScreenPos(contentPos);
		}

		// Text
		{
			Vector2 framePadding = UI::GetTheme().Style.FramePadding;

			UI::ShiftCursorX(framePadding.x);

			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + itemSize.x - framePadding.x);

			float actualSize = ImGui::CalcTextSize(fileName.data()).x + framePadding.x * 2.0f;
			float avail = ImGui::GetContentRegionAvail().x;

			float off = (avail - actualSize) * 0.5f;
			if (off > 0.0f)
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

			ImGui::TextWrapped(fileName.data());
			ImGui::PopTextWrapPos();
		}

		ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + itemSize.y));

		if (ImGui::BeginPopup("CBItemPopup"))
		{
			if (ImGui::MenuItem("Open In Explorer"))
			{
				Platform::OpenInFileExplorer(GetFilePath());
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}


	CBAssetItem::CBAssetItem(ContentBrowserPanel* panel, const FilePath& path)
		: CBItem(panel, path)
	{
		FilePath relativePath = AssetManager::GetAssetRelativePath(path);
		AssetHandle handle = Project::GetEditorAssetManager()->GetAssetHandleFromFilePath(relativePath);
		AssetType type = Project::GetEditorAssetManager()->GetAssetType(handle);

		m_Payload.AssetHandle = handle;
		m_Payload.AssetType = type;
		m_Payload.FilePath = GetFilePath();
	}

	void CBAssetItem::OnImGuiRender(ImVec2 itemSize)
	{
		const String& fileName = GetFileName();
		String stem = FilePath(fileName).stem().string();

		TrackMouseState(itemSize);
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();

		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", &m_Payload, sizeof(m_Payload), ImGuiCond_Once);
			ImGui::Text(stem.c_str());
			ImGui::EndDragDropSource();
		}

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// Shadow
		{
			const ImVec2 shadowOffset = ImVec2(1.5f, 1.5f); 
			const ImU32 shadowColor = IM_COL32(0, 0, 0, 255);

			drawList->AddRectFilled(ImVec2(cursorPos.x + shadowOffset.x, cursorPos.y + shadowOffset.y),
				ImVec2(cursorPos.x + itemSize.x + shadowOffset.x, cursorPos.y + itemSize.y + shadowOffset.y),
				shadowColor, 3.0f);
		}

		// Background
		{
			const ImU32 selectionColor = UI::GetTheme().Accent;
			bool isSelected = m_State.IsSet(CBItemStateFlag_Selected);
			bool isHovered = m_State.IsSet(CBItemStateFlag_Hovered);

			const ImU32 borderColor = isSelected ? selectionColor : UI::GetTheme().TabActive;
			const ImU32 bgColor = isSelected ? selectionColor : UI::GetTheme().HeaderActive;
			const ImU32 imageBgColor = UI::GetTheme().BackgroundDark;

			drawList->AddRectFilled(cursorPos,
				ImVec2(cursorPos.x + itemSize.x, cursorPos.y + itemSize.y),
				bgColor, 3.0f);

			drawList->AddRectFilled(cursorPos,
				ImVec2(cursorPos.x + itemSize.x, cursorPos.y + itemSize.y * IMAGE_TO_ITEM_RATIO),
				imageBgColor, 3.0f);

			if (isSelected || isHovered)
			{
				drawList->AddRect(ImVec2(cursorPos.x, cursorPos.y),
					ImVec2(cursorPos.x + itemSize.x, cursorPos.y + itemSize.y),
					borderColor, 3.0f);
			}
		}

		// Image
		{
			const float imagePadding = 4.f;

			ImVec2 iconSize = { itemSize.x - 2.f * imagePadding, itemSize.y * IMAGE_TO_ITEM_RATIO - 2.f * imagePadding };
			ImVec2 contentPos = cursorPos;
			contentPos.x += imagePadding;
			contentPos.y += imagePadding;

			// TODO: Thumbnails / icons per asset type
			 
			//drawList->AddImage(UI::GetTextureID(TextureGenerator::GetWhiteTexture()),
			//	contentPos,
			//	ImVec2(contentPos.x + iconSize.x, contentPos.y + iconSize.y));

			contentPos.y += iconSize.y + imagePadding;
			contentPos.x -= imagePadding;
			ImGui::SetCursorScreenPos(contentPos);
		}
		
		// Text
		{
			Vector2 framePadding = UI::GetTheme().Style.FramePadding;

			UI::ShiftCursorX(framePadding.x);

			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + itemSize.x - framePadding.x);
			ImGui::TextWrapped(stem.c_str());
			ImGui::PopTextWrapPos();

			String assetType = Utils::ToUpper((String)Utils::AssetTypeToString(m_Payload.AssetType));
			if (m_Payload.AssetType == AssetType::EnvironmentMap)
				assetType = "ENVMAP";

			ImVec2 textSize = ImGui::CalcTextSize(assetType.data());
			ImVec2 localCursorPos = { itemSize.x - textSize.x - framePadding.x, itemSize.y - textSize.y - framePadding.y };
			ImGui::SetCursorScreenPos({ cursorPos.x + localCursorPos.x, cursorPos.y + localCursorPos.y });
			
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 125));
			ImGui::Text(assetType.data());
			ImGui::PopStyleColor();
		}

		ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + itemSize.y));

		if (ImGui::BeginPopup("CBItemPopup"))
		{
			if (ImGui::MenuItem("Open Externally"))
			{
				Platform::OpenFileExternally(GetFilePath());
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (m_State.IsSet(CBItemStateFlag_Active))
		{
			// On double click
		}
	}


	ContentBrowserPanel::ContentBrowserPanel(const Ref<EditorContext>& context)
		: Panel(CONTENT_BROWSER_PANEL_ID, context)
	{
		Refresh();
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("ContentBrowser");
		
		RenderHeadBar();

		ImGui::Separator();

		m_IsAnyItemHovered = false;

		const std::vector<Ref<CBItem>>& itemList = !m_SearchString.empty() ? m_SearchResult : m_CurrentFolder->GetChildren();

		float cellSize = 2 * m_Padding + m_ItemSize.x;
		float panelWidth = ImGui::GetContentRegionAvail().x;
		uint32 columnCount = (uint32)((panelWidth - 2 * m_Padding) / cellSize);
		if (columnCount < 1) 
			columnCount = 1;

		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(m_Padding, ImGui::GetStyle().CellPadding.y));
		ImGui::BeginTable("ContentBrowserItems", columnCount, ImGuiTableFlags_SizingFixedSame);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);

		for (const auto& item: itemList)
		{
			ImGui::TableNextColumn();
			ImGui::PushID(item->GetFileName().data());

			item->OnImGuiRender(m_ItemSize);

			ImGui::PopID();

			bool isHovered = item->GetState().IsSet(CBItemStateFlag_Hovered);
			if (isHovered)
				m_IsAnyItemHovered = true;

			bool isSelected = item->GetState().IsSet(CBItemStateFlag_Selected);
			if (isSelected)
				m_SelectedItem = item;

			if (item->IsFolder())
			{
				if (item->GetState().IsSet(CBItemStateFlag_Active))
				{
					m_SearchResult.clear();
					m_SearchString.clear();
					m_CurrentFolder = item.As<CBFolder>();
					break;
				}
			}
		}
		ImGui::PopStyleVar();
		ImGui::EndTable();
		ImGui::PopStyleVar();

		if (ImGui::IsWindowHovered() && !m_IsAnyItemHovered)
		{
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				DeselectItem();

				for (const auto& item : itemList)
				{
					item->GetState().Clear(CBItemStateFlag_Selected);
				}
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Always, ImVec2(0.f, 1.0f));
				ImGui::OpenPopup("CBPopup");
			}
		}

		if (ImGui::BeginPopup("CBPopup"))
		{
			if (ImGui::MenuItem("Open In Explorer"))
			{
				Platform::OpenInFileExplorer(m_CurrentFolder->GetFilePath());
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::End();
	}

	void ContentBrowserPanel::DeselectItem()
	{
		if (m_SelectedItem)
		{
			m_SelectedItem->GetState().Clear(CBItemStateFlag_Selected);
			m_SelectedItem = nullptr;
		}
	}

	void ContentBrowserPanel::RenderHeadBar()
	{
		ImVec2 regionAvail = ImGui::GetContentRegionAvail();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 5.f, 10.f });
		ImGui::PushStyleColor(ImGuiCol_Button, UI::GetTheme().BackgroundDark);

		if (ImGui::ImageButton(UI::GetTextureID(EditorResources::GetIcon("ContentBrowser_Undo")), m_ButtonSize))
		{
			if (m_CurrentFolder != m_RootFolder)
			{
				m_CurrentFolder = FindItemParentFolder(m_CurrentFolder);
			}
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(UI::GetTextureID(EditorResources::GetIcon("ContentBrowser_Redo")), m_ButtonSize))
		{
			// TODO
		}

		ImGui::SameLine();
		UI::ShiftCursorX(2.f);

		if (ImGui::ImageButton(UI::GetTextureID(EditorResources::GetIcon("ContentBrowser_Refresh")), m_ButtonSize))
		{
			Refresh();
		}

		ImGui::SameLine();
		UI::ShiftCursorX(2.f);

		ImGui::PushItemWidth(regionAvail.x * 0.2f);

		String oldString = m_SearchString;
		UI::TextInputWithHint("SearchTextInput", "Search...", m_SearchString);

		if (oldString != m_SearchString && !m_SearchString.empty())
			FilterSearch();

		ImGui::SameLine();

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + regionAvail.x * 0.01f);
		ImGui::Text(m_CurrentFolder->GetFilePath().c_str());

		// Settings
		{
			ImGui::SameLine(regionAvail.x - m_ButtonSize.x);

			ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0 });
			ImGui::PushStyleColor(ImGuiCol_Border, { 0, 0, 0, 0 });
			if (ImGui::ImageButton(UI::GetTextureID(EditorResources::GetIcon("Settings")), m_ButtonSize))
				ImGui::OpenPopup("ContentBrowserSettings");
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();

			if (ImGui::BeginPopup("ContentBrowserSettings"))
			{
				float itemSize = m_ItemSize.y;
				if(ImGui::SliderFloat("Item Size", &itemSize, 125, 500));
				{
					m_ItemSize = { itemSize * IMAGE_TO_ITEM_RATIO, itemSize };
				}
				ImGui::EndPopup();
			}
		}

		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}


	void ContentBrowserPanel::Refresh()
	{
		FilePath assetDir = Project::GetAssetDirectory();
		String currentFilePath = m_CurrentFolder != nullptr ? m_CurrentFolder->GetFilePath() : String();

		m_RootFolder = Ref<CBFolder>::Create(this, assetDir);

		if(!currentFilePath.empty())
			m_CurrentFolder = FindItemByFilePath(m_RootFolder, currentFilePath);

		if (m_CurrentFolder == nullptr)
			m_CurrentFolder = m_RootFolder;
	}

	Ref<CBFolder> ContentBrowserPanel::FindItemParentFolder(const Ref<CBItem>& item)
	{
		const String& path = item->GetFilePath();
		std::queue<Ref<CBFolder>> searchQueue;
		searchQueue.push(m_RootFolder);

		do
		{
			Ref<CBFolder> currentRoot = searchQueue.front();
			searchQueue.pop();

			for (auto& child : currentRoot->GetChildren())
			{
				if (child->GetFilePath() == path)
					return currentRoot;

				if (child->IsFolder())
					searchQueue.push(child);
			}
		} while (!searchQueue.empty());

		return m_RootFolder;
	}

	Ref<CBItem> ContentBrowserPanel::FindItemByFilePath(const Ref<CBFolder>& folder, const String& path)
	{
		for (const auto& child : folder->GetChildren())
		{
			auto itemPath = child->GetFilePath();
			if (child->GetFilePath() == path)
				return child;

			if (child->IsFolder())
			{
				Ref<CBItem> result = FindItemByFilePath(child, path);
				if (result)
					return result;
			}
		}

		return nullptr;
	}

	void ContentBrowserPanel::FilterSearch()
	{
		m_SearchResult.clear();
		String searchString = Utils::ToLower(m_SearchString);

		std::queue<Ref<CBItem>> searchQueue;

		for (auto& child : m_RootFolder->GetChildren())
			searchQueue.push(child);

		while (!searchQueue.empty())
		{
			Ref<CBItem> item = searchQueue.front();

			const String filename = Utils::ToLower(item->GetFileName());
			if (filename.find(searchString) != std::string::npos)
			{
				m_SearchResult.push_back(item);
			}
			else if (!item->IsFolder())
			{
				Ref<CBAssetItem> assetItem = item.As<CBAssetItem>();
				String assetType = Utils::ToLower((String)Utils::AssetTypeToString(assetItem->GetAssetType()));

				if (assetType.find(searchString) != std::string::npos)
					m_SearchResult.push_back(item);
			}

			if (item->IsFolder())
			{
				Ref<CBFolder> folder = item;
				for (auto& child : folder->GetChildren())
					searchQueue.push(child);
			}

			searchQueue.pop();
		}

		std::sort(m_SearchResult.begin(), m_SearchResult.end(), [this](const Ref<CBItem>& left, const Ref<CBItem>& right)
			{
				return left->LexCompare(right);
			});
	}
}
