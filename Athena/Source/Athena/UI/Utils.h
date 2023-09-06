#pragma once

#include "Athena/Core/Core.h"

#include <ImGui/imgui.h>


namespace Athena::UI
{
	ATHENA_API void ShiftCursorX(float offset);
	ATHENA_API void ShiftCursorY(float offset);
	ATHENA_API void ShiftCursor(float offsetX, float offsetY);

	ATHENA_API ImColor MultiplyColorByScalar(const ImColor& color, float scalar);

	ATHENA_API const ImVec4& GetDarkColor();

	enum class Fonts
	{
		Default = 0,
		Bold = 1,
		Default22 = 2,
		Default16 = Default
	};

	ATHENA_API void PushFont(Fonts font);
	ATHENA_API void PopFont();
}
