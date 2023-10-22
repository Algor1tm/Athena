#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"

#include "Panels/Panel.h"

#include <ImGui/imgui.h>


namespace Athena
{
	class Texture2D;

	class ContentBrowserPanel : public Panel
	{
	public:
		ContentBrowserPanel(std::string_view name, const Ref<EditorContext>& context);

		virtual void OnImGuiRender() override;

	private:
		struct TreeNode
		{
			bool IsFolder;
			String FilePath;
			String FileName;
			std::vector<TreeNode> Children;
			TreeNode* ParentNode;
		};

	private:
		void Refresh();
		void ReloadTreeHierarchy(const FilePath& srcDirectory, TreeNode& dstNode);
		TreeNode* FindTreeNode(TreeNode& root, const String& path);

		void Search();

	private:
		TreeNode m_TreeRoot;
		TreeNode* m_CurrentNode;

		String m_SearchString;
		std::vector<TreeNode*> m_SearchResult;

		const std::string_view m_AssetDirectory = "Assets";

		const ImVec2 m_ButtonSize = { 16.f, 16.f };
		const ImVec2 m_ItemSize = { 96.f, 96.f };
		const float m_Padding = 8.f;
	};
}
