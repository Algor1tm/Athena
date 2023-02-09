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

		Ref<Texture2D> m_FolderIcon;
		Ref<Texture2D> m_FileIcon;
		Ref<Texture2D> m_BackButton;
		Ref<Texture2D> m_ForwardButton;

		static constexpr Vector2 m_BackButtonSize = { 16.f, 16.f };
		static constexpr Vector2 m_ItemSize = { 96.f, 96.f };
		static constexpr float m_Padding = 8.f;
	};
}
