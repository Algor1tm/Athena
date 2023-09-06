#include "Utils.h"

#include "Athena/Math/Common.h"

#include <ImGui/imgui_internal.h>


namespace Athena::UI
{
	void ShiftCursorX(float offset)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
	}

	void ShiftCursorY(float offset)
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
	}

	void ShiftCursor(float offsetX, float offsetY)
	{
		ShiftCursorX(offsetX);
		ShiftCursorY(offsetY);
	}

	ImColor MultiplyColorByScalar(const ImColor& color, float scalar)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, sat, Math::Min(val * scalar, 1.0f));
	}

	const ImVec4& GetDarkColor()
	{
		return ImGui::GetStyle().Colors[ImGuiCol_TitleBg];
	}

	void PushFont(Fonts font)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImFont* imguiFont = io.FontDefault;

		switch (font)
		{
		case Fonts::Default: imguiFont = io.FontDefault; break;
		case Fonts::Bold: imguiFont = io.Fonts->Fonts[1]; break;
		case Fonts::Default22: imguiFont = io.Fonts->Fonts[2]; break;
		}

		ImGui::PushFont(imguiFont);
	}

	void PopFont()
	{
		ImGui::PopFont();
	}
}
