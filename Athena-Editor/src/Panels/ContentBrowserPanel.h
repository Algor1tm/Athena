#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"
#include "UI/Icon.h"

#include <ImGui/imgui.h>

#include <filesystem>


namespace Athena
{
	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender();

	private:
		std::filesystem::path m_CurrentDirectory;
		std::string_view m_AssetDirectory = "assets";

		UI::Icon m_FolderIcon;
		UI::Icon m_FileIcon;
		UI::Icon m_BackButtonIcon;

		static constexpr ImVec2 m_BackButtonSize = { 16.f, 16.f };
		static constexpr ImVec2 m_ItemSize = { 96.f, 96.f };
		static constexpr float m_Padding = 8.f;
	};
}
