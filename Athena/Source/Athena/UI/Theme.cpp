#include "Theme.h"

#include "Athena/Core/Application.h"
#include "Athena/ImGui/ImGuiLayer.h"

#include "Athena/UI/UI.h"


namespace Athena::UI
{
	Theme Theme::DefaultDark()
	{
		Theme theme;

		theme.Style.FrameRounding = 2.5f;
		theme.Style.FrameBorderSize = 1.f;
		theme.Style.ScrollbarSize = 11.f;
		theme.Style.WindowRounding = 5.f;
		theme.Style.FramePadding = { 2.35f, 2.35f };

		theme.Titlebar = IM_COL32(21, 21, 21, 255);
		theme.Background = IM_COL32(27, 27, 27, 255);
		theme.BackgroundDark = IM_COL32(17, 17, 17, 255);
		theme.BackgroundPopup = IM_COL32(42, 42, 42, 255);
		
		theme.Accent = IM_COL32(0, 112, 224, 255);
		theme.Text = IM_COL32(255, 255, 255, 255);
		theme.ErrorText = IM_COL32(240, 40, 40, 255);
		
		theme.FrameBg = IM_COL32(8, 8, 8, 255);
		theme.FrameBgActive = IM_COL32(8, 8, 8, 255);
		theme.Header = IM_COL32(40, 40, 40, 255);
		theme.HeaderActive = IM_COL32(53, 53, 53, 255);
		theme.Button = IM_COL32(27, 27, 27, 255);
		theme.ButtonActive = IM_COL32(27, 27, 27, 255);
		theme.Tab = theme.Background;
		theme.TabActive = IM_COL32(28, 69, 143, 120);

		return theme;
	}

	Theme& GetTheme()
	{
		return Application::Get().GetImGuiLayer()->GetTheme();
	}

	void ThemeEditor(Theme& theme)
	{
		UI::TextCentered("Style");
		if (UI::BeginPropertyTable())
		{
			UI::PropertyDrag("FrameRounding", &theme.Style.FrameRounding, 0.05f);
			UI::PropertyDrag("FrameBorderSize", &theme.Style.FrameBorderSize, 0.05f);
			UI::PropertyDrag("ScrollbarSize", &theme.Style.ScrollbarSize, 0.05f);
			UI::PropertyDrag("WindowRounding", &theme.Style.WindowRounding, 0.05f);
			UI::PropertyDrag("FramePadding", &theme.Style.FramePadding, 0.05f);

			UI::EndPropertyTable();
		}

		ImGui::Spacing();
		UI::TextCentered("Colors");
		if (UI::BeginPropertyTable())
		{
			UI::PropertyColorU32("Titlebar", &theme.Titlebar);
			UI::PropertyColorU32("Background", &theme.Background);
			UI::PropertyColorU32("BackgroundDark", &theme.BackgroundDark);
			UI::PropertyColorU32("BackgroundPopup", &theme.BackgroundPopup);
			UI::PropertyColorU32("Accent", &theme.Accent);
			UI::PropertyColorU32("Text", &theme.Text);
			UI::PropertyColorU32("FrameBg", &theme.FrameBg);
			UI::PropertyColorU32("FrameBgActive", &theme.FrameBgActive);
			UI::PropertyColorU32("Header", &theme.Header);
			UI::PropertyColorU32("HeaderActive", &theme.HeaderActive);
			UI::PropertyColorU32("Button", &theme.Button);
			UI::PropertyColorU32("ButtonActive", &theme.ButtonActive);
			UI::PropertyColorU32("Tab", &theme.Tab);
			UI::PropertyColorU32("TabActive", &theme.TabActive);

			UI::EndPropertyTable();
		}
	}
}
