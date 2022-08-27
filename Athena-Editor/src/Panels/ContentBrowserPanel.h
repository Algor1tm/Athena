#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"

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

		Ref<Texture2D> m_FolderIcon;
		RendererID m_FolderIconID;

		Ref<Texture2D> m_FileIcon;
		RendererID m_FileIconID;

		Ref<Texture2D> m_BackButtonIcon;
		RendererID m_BackButtonIconID;

		static constexpr ImVec2 m_BackButtonSize = { 16.f, 16.f };
		static constexpr ImVec2 m_ItemSize = { 96.f, 96.f };
		static constexpr float m_Padding = 8.f;
	};
}
