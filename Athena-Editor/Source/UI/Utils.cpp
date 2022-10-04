#include "Utils.h"

#include <ImGui/imgui_internal.h>


namespace Athena::UI
{
	void PushBoldFont()
	{
		ImGuiIO& io = ImGui::GetIO();
		auto& boldFont = io.Fonts->Fonts[0];
		ImGui::PushFont(boldFont);
	}
}
