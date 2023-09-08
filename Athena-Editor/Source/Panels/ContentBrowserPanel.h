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
		ContentBrowserPanel(std::string_view name);

		virtual void OnImGuiRender() override;

	private:
		FilePath m_CurrentDirectory;
		std::string_view m_AssetDirectory = "Assets";
		FilePath m_LastDirectory;

		static constexpr ImVec2 m_UndoButtonSize = { 16.f, 16.f };
		static constexpr ImVec2 m_ItemSize = { 96.f, 96.f };
		static constexpr float m_Padding = 8.f;
	};
}
