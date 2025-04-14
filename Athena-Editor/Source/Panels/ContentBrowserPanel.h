#pragma once

#include "Athena/Asset/Asset.h"
#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"

#include "Panels/Panel.h"

#include <ImGui/imgui.h>


namespace Athena
{
#define IMAGE_TO_ITEM_RATIO 0.66f

	struct CBDragDropPayload
	{
		AssetHandle AssetHandle = 0;
		AssetType AssetType = AssetType::None;
		FilePath FilePath;
	};

	enum CBItemStateFlags
	{
		CBItemStateFlag_Default = BIT(0),
		CBItemStateFlag_Hovered = BIT(1),
		CBItemStateFlag_Selected = BIT(2),
		CBItemStateFlag_Active = BIT(3),
		CBItemStateFlag_ActivePopup = BIT(4),
	};

	struct CBItemState
	{
		void Set(CBItemStateFlags flag)
		{
			BitField = BitField | flag;
		}

		void Clear(CBItemStateFlags flag)
		{
			BitField = BitField & ~(flag);
		}

		void SetIf(bool condition, CBItemStateFlags flag)
		{
			if (condition)
				Set(flag);
			else
				Clear(flag);
		}

		bool IsSet(CBItemStateFlags flag) const
		{
			return BitField & flag;
		}

		uint32 BitField = CBItemStateFlag_Default;
	};

	class ContentBrowserPanel;

	class CBItem
	{
	public:
		CBItem(ContentBrowserPanel* panel, const FilePath& path);

		virtual void OnImGuiRender(ImVec2 itemSize) = 0;
		virtual bool IsFolder() const = 0;

		void TrackMouseState(ImVec2 itemSize);

		CBItemState& GetState() { return m_State; }

		const String& GetFilePath() const { return m_FilePath; }
		const String& GetFileName() const { return m_FileName; }

		bool LexCompare(const Ref<CBItem>& item) const;

	protected:
		String m_FilePath;
		String m_FileName;
		CBItemState m_State;
		ContentBrowserPanel* m_ContentBrowserPanel = nullptr;
	};


	class CBFolder: public CBItem
	{
	public:
		CBFolder(ContentBrowserPanel* panel, const FilePath& path);

		virtual void OnImGuiRender(ImVec2 itemSize) override;
		virtual bool IsFolder() const override { return true; }

		const std::vector<Ref<CBItem>>& GetChildren() const { return m_ChildrenItems; }

	private:
		std::vector<Ref<CBItem>> m_ChildrenItems;
	};


	class CBAssetItem: public CBItem
	{
	public:
		CBAssetItem(ContentBrowserPanel* panel, const FilePath& path);

		virtual void OnImGuiRender(ImVec2 itemSize) override;
		virtual bool IsFolder() const override { return false; }

		AssetHandle GetHandle() const { return m_Payload.AssetHandle; }
		AssetType GetAssetType() const { return m_Payload.AssetType; }

	private:
		CBDragDropPayload m_Payload;
	};


	class ContentBrowserPanel : public Panel
	{
	public:
		ContentBrowserPanel(const Ref<EditorContext>& context);

		virtual void OnImGuiRender() override;
		void DeselectItem();
		void Refresh();

	private:
		void RenderHeadBar();

		Ref<CBFolder> FindItemParentFolder(const Ref<CBItem>& item);
		Ref<CBItem> FindItemByFilePath(const Ref<CBFolder>& folder, const String& path);
		void FilterSearch();

	private:
		Ref<CBFolder> m_RootFolder;
		Ref<CBFolder> m_CurrentFolder;
		Ref<CBItem> m_SelectedItem;

		bool m_IsAnyItemHovered = false;

		String m_SearchString;
		std::vector<Ref<CBItem>> m_SearchResult;

		const ImVec2 m_ButtonSize = { 16.f, 16.f };
		ImVec2 m_ItemSize = { 180.f * IMAGE_TO_ITEM_RATIO, 180.f };
		const float m_Padding = 8.f;
	};
}
