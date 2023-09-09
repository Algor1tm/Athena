#pragma once

#include "Athena/Core/Core.h"

#include <ImGui/imgui.h>


namespace Athena::UI
{
	struct Style
	{
		float FrameRounding;
		float FrameBorderSize;
		float ScrollbarSize;
		float WindowRounding;
	};


	struct ATHENA_API Theme
	{
		static Theme Dark();

		Style Style;

		ImU32 Titlebar;
		ImU32 Background;
		ImU32 BackgroundDark;
		ImU32 BackgroundPopup;

		ImU32 Accent;
		ImU32 Text;

		ImU32 FrameBg;
		ImU32 FrameBgActive;
		ImU32 Header;
		ImU32 HeaderActive;
		ImU32 Button;
		ImU32 ButtonActive;
		ImU32 Tab;
		ImU32 TabActive;
	};

	ATHENA_API Theme& GetTheme();
	ATHENA_API void ThemeEditor(Theme& theme);
}
