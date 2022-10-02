#pragma once

#include "Athena/Math/Vector.h"
#include "Athena/Core/Color.h"
#include "Athena/Renderer/Texture.h"

#include <ImGui/imgui.h>

#include <string_view>


namespace Athena::UI
{
	void DrawVec3Controller(std::string_view label, Vector3& values, float defaultValues, float columnWidth = 70.f);
	
	bool TextInput(const String& label, String& destination);

	template <typename Controller>
	bool DrawController(std::string_view label, float offset, Controller controller)
	{
		ImGui::Text(label.data());
		ImGui::SameLine(offset);

		return controller();
	}
}
