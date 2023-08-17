#pragma once

#include "Athena/Core/Core.h"

#include <ImGui/imgui.h>


namespace Athena::UI
{
	ATHENA_API void PushBoldFont();

	ATHENA_API void ShiftCursorX(float offset);
	ATHENA_API void ShiftCursorY(float offset);
	ATHENA_API void ShiftCursor(float offsetX, float offsetY);

	ATHENA_API const ImVec4& GetDarkColor();
}
