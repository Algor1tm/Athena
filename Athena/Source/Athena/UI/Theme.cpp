#include "Theme.h"

#include "Athena/Core/Application.h"
#include "Athena/ImGui/ImGuiLayer.h"

#include "Athena/UI/UI.h"


namespace Athena::UI
{
	Theme Theme::Dark()
	{
		Theme theme;

		theme.Style.FrameRounding = 2.5f;
		theme.Style.FrameBorderSize = 1.f;
		theme.Style.ScrollbarSize = 11.f;
		theme.Style.WindowRounding = 5.f;

		theme.Titlebar = IM_COL32(21, 21, 21, 255);
		theme.Background = IM_COL32(30, 30, 30, 255);
		theme.BackgroundDark = IM_COL32(17, 17, 17, 255);
		theme.BackgroundPopup = theme.Background;
		
		theme.Accent = IM_COL32(0, 112, 224, 255);
		theme.Text = IM_COL32(255, 255, 255, 255);
		
		theme.FrameBg = IM_COL32(14, 14, 14, 255);
		theme.FrameBgActive = IM_COL32(51, 51, 51, 255);
		theme.Header = IM_COL32(40, 40, 40, 255);
		theme.HeaderActive = IM_COL32(53, 53, 53, 255);
		theme.Button = IM_COL32(41, 41, 41, 255);
		theme.ButtonActive = IM_COL32(53, 53, 53, 255);
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

			UI::EndPropertyTable();
		}

		ImGui::Spacing();
		UI::TextCentered("Colors");
		if (UI::BeginPropertyTable())
		{
			ImVec4 color4;

			color4 = ImGui::ColorConvertU32ToFloat4(theme.Titlebar);
			UI::PropertyColor4("Titlebar", &color4.x);
			theme.Titlebar = ImGui::ColorConvertFloat4ToU32(color4);

			color4 = ImGui::ColorConvertU32ToFloat4(theme.Background);
			UI::PropertyColor4("Background", &color4.x);
			theme.Background = ImGui::ColorConvertFloat4ToU32(color4);

			color4 = ImGui::ColorConvertU32ToFloat4(theme.BackgroundDark);
			UI::PropertyColor4("BackgroundDark", &color4.x);
			theme.BackgroundDark = ImGui::ColorConvertFloat4ToU32(color4);

			color4 = ImGui::ColorConvertU32ToFloat4(theme.BackgroundPopup);
			UI::PropertyColor4("BackgroundPopup", &color4.x);
			theme.BackgroundPopup = ImGui::ColorConvertFloat4ToU32(color4);

			color4 = ImGui::ColorConvertU32ToFloat4(theme.Accent);
			UI::PropertyColor4("Accent", &color4.x);
			theme.Accent = ImGui::ColorConvertFloat4ToU32(color4);

			color4 = ImGui::ColorConvertU32ToFloat4(theme.Text);
			UI::PropertyColor4("Text", &color4.x);
			theme.Text = ImGui::ColorConvertFloat4ToU32(color4);

			color4 = ImGui::ColorConvertU32ToFloat4(theme.FrameBg);
			UI::PropertyColor4("FrameBg", &color4.x);
			theme.FrameBg = ImGui::ColorConvertFloat4ToU32(color4);

			color4 = ImGui::ColorConvertU32ToFloat4(theme.FrameBgActive);
			UI::PropertyColor4("FrameBgActive", &color4.x);
			theme.FrameBgActive = ImGui::ColorConvertFloat4ToU32(color4);

			color4 = ImGui::ColorConvertU32ToFloat4(theme.Button);
			UI::PropertyColor4("Button", &color4.x);
			theme.Button = ImGui::ColorConvertFloat4ToU32(color4);

			color4 = ImGui::ColorConvertU32ToFloat4(theme.ButtonActive);
			UI::PropertyColor4("ButtonActive", &color4.x);
			theme.ButtonActive = ImGui::ColorConvertFloat4ToU32(color4);

			color4 = ImGui::ColorConvertU32ToFloat4(theme.Tab);
			UI::PropertyColor4("Tab", &color4.x);
			theme.Tab = ImGui::ColorConvertFloat4ToU32(color4);

			color4 = ImGui::ColorConvertU32ToFloat4(theme.TabActive);
			UI::PropertyColor4("TabActive", &color4.x);
			theme.TabActive = ImGui::ColorConvertFloat4ToU32(color4);

			UI::EndPropertyTable();
		}
	}
}
