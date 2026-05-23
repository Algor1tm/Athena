#pragma once

#include "Athena/Asset/Asset.h"
#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"

#include "Panels/Panel.h"

#include <ImGui/imgui.h>


namespace Athena
{
#define IMAGE_TO_ITEM_RATIO 0.66f

	class CBAssetItem;

	struct CBDragDropPayload
	{
		AssetHandle AssetHandle = 0;
		AssetType AssetType = AssetType::None;
		FilePath FilePath;
		CBAssetItem* Item = nullptr;
	};

	enum CBItemStateFlags
	{
		CBItemStateFlag_Default = BIT(0),
		CBItemStateFlag_Hovered = BIT(1),
		CBItemStateFlag_Selected = BIT(2),
		CBItemStateFlag_Active = BIT(3),
		CBItemStateFlag_ActivePopup = BIT(4),
		CBItemStateFlag_Rename = BIT(6),
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

	class CBItem: public RefCounted
	{
	public:
		CBItem(ContentBrowserPanel* panel, const FilePath& path);

		virtual void OnImGuiRender(ImVec2 itemSize) = 0;
		virtual bool IsFolder() const = 0;
		virtual void Move(const FilePath& path) = 0;

		void OnRename();
		void TrackMouseState(ImVec2 itemSize);

		CBItemState& GetState() { return m_State; }

		const String& GetFilePath() const { return m_FilePath; }
		const String& GetFileName() const { return m_FileName; }

		bool LexCompare(const Ref<CBItem>& item) const;

	protected:
		String m_FilePath;
		String m_FileName;
		String m_RenameBuffer;
		CBItemState m_State;
		ContentBrowserPanel* m_ContentBrowserPanel = nullptr;
	};


	class CBFolder: public CBItem
	{
	public:
		CBFolder(ContentBrowserPanel* panel, const FilePath& path);

		virtual void OnImGuiRender(ImVec2 itemSize) override;
		virtual bool IsFolder() const override { return true; }
		virtual void Move(const FilePath& path) override;

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
		virtual void Move(const FilePath& path) override;

		AssetHandle GetAssetHandle() const { return m_Payload.AssetHandle; }
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
		void QueueRefresh() { m_QueueRefresh = true; }

		void SetMoveItem(CBItem* moveItem) { m_MoveItem = moveItem; }
		CBItem* GetMoveItem() { return m_MoveItem; }

		Ref<CBFolder> GetCurrentFolder() const { return m_CurrentFolder; }

	private:
		FilePath CreateUniqueFile(const String& name, const String& ext);
		void RenderHeadBar();

		Ref<CBFolder> FindItemParentFolder(const Ref<CBItem>& item);
		Ref<CBItem> FindItemByFilePath(const Ref<CBFolder>& folder, const String& path);
		void FilterSearch();

	private:
		Ref<CBFolder> m_RootFolder;
		Ref<CBFolder> m_CurrentFolder;
		Ref<CBItem> m_SelectedItem;
		CBItem* m_MoveItem = nullptr;

		bool m_IsAnyItemHovered = false;
		bool m_QueueRefresh = false;

		String m_SearchString;
		std::vector<Ref<CBItem>> m_SearchResult;

		ImVec2 m_ButtonSize = { 16.f, 16.f };
		ImVec2 m_ItemSize = { 170.f * IMAGE_TO_ITEM_RATIO, 170.f };
		float m_Padding = 8.f;
	};
}
