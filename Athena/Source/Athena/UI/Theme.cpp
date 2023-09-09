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
			UI::Property("FrameRounding");
			ImGui::DragFloat("##FrameRounding", &theme.Style.FrameRounding, 0.05f);

			UI::Property("FrameBorderSize");
			ImGui::DragFloat("##FrameBorderSize", &theme.Style.FrameBorderSize, 0.05f);

			UI::Property("ScrollbarSize");
			ImGui::DragFloat("##ScrollbarSize", &theme.Style.ScrollbarSize, 0.05f);

			UI::Property("WindowRounding");
			ImGui::DragFloat("##WindowRounding", &theme.Style.WindowRounding, 0.05f);

			UI::EndPropertyTable();
		}

		ImGui::Spacing();
		UI::TextCentered("Colors");
		if (UI::BeginPropertyTable())
		{
			ImVec4 color4;

			UI::Property("Titlebar");
			color4 = ImGui::ColorConvertU32ToFloat4(theme.Titlebar);
			ImGui::ColorEdit4("##Titlebar", &color4.x);
			theme.Titlebar = ImGui::ColorConvertFloat4ToU32(color4);

			UI::Property("Background");
			color4 = ImGui::ColorConvertU32ToFloat4(theme.Background);
			ImGui::ColorEdit4("##Background", &color4.x);
			theme.Background = ImGui::ColorConvertFloat4ToU32(color4);

			UI::Property("BackgroundDark");
			color4 = ImGui::ColorConvertU32ToFloat4(theme.BackgroundDark);
			ImGui::ColorEdit4("##BackgroundDark", &color4.x);
			theme.BackgroundDark = ImGui::ColorConvertFloat4ToU32(color4);

			UI::Property("BackgroundPopup");
			color4 = ImGui::ColorConvertU32ToFloat4(theme.BackgroundPopup);
			ImGui::ColorEdit4("##BackgroundPopup", &color4.x);
			theme.BackgroundPopup = ImGui::ColorConvertFloat4ToU32(color4);

			UI::Property("Accent");
			color4 = ImGui::ColorConvertU32ToFloat4(theme.Accent);
			ImGui::ColorEdit4("##Accent", &color4.x);
			theme.Accent = ImGui::ColorConvertFloat4ToU32(color4);

			UI::Property("Text");
			color4 = ImGui::ColorConvertU32ToFloat4(theme.Text);
			ImGui::ColorEdit4("##Text", &color4.x);
			theme.Text = ImGui::ColorConvertFloat4ToU32(color4);

			UI::Property("FrameBg");
			color4 = ImGui::ColorConvertU32ToFloat4(theme.FrameBg);
			ImGui::ColorEdit4("##FrameBg", &color4.x);
			theme.FrameBg = ImGui::ColorConvertFloat4ToU32(color4);

			UI::Property("FrameBgActive");
			color4 = ImGui::ColorConvertU32ToFloat4(theme.FrameBgActive);
			ImGui::ColorEdit4("##FrameBgActive", &color4.x);
			theme.FrameBgActive = ImGui::ColorConvertFloat4ToU32(color4);

			UI::Property("Button");
			color4 = ImGui::ColorConvertU32ToFloat4(theme.Button);
			ImGui::ColorEdit4("##Button", &color4.x);
			theme.Button = ImGui::ColorConvertFloat4ToU32(color4);

			UI::Property("ButtonActive");
			color4 = ImGui::ColorConvertU32ToFloat4(theme.ButtonActive);
			ImGui::ColorEdit4("##ButtonActive", &color4.x);
			theme.ButtonActive = ImGui::ColorConvertFloat4ToU32(color4);

			UI::Property("Tab");
			color4 = ImGui::ColorConvertU32ToFloat4(theme.Tab);
			ImGui::ColorEdit4("##Tab", &color4.x);
			theme.Tab = ImGui::ColorConvertFloat4ToU32(color4);

			UI::Property("TabActive");
			color4 = ImGui::ColorConvertU32ToFloat4(theme.TabActive);
			ImGui::ColorEdit4("##TabActive", &color4.x);
			theme.TabActive = ImGui::ColorConvertFloat4ToU32(color4);

			UI::EndPropertyTable();
		}
	}
}
