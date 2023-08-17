#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Math/Vector.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Texture.h"

#include "Athena/UI/Utils.h"

#include <ImGui/imgui.h>

#include <string_view>


namespace Athena::UI
{
	ATHENA_API bool TextInput(const String& label, String& destination, ImGuiInputTextFlags flags = 0);
	ATHENA_API bool TextInputWithHint(const std::string_view hint, String& destination, ImGuiInputTextFlags flags = 0);


	ATHENA_API bool TreeNode(std::string_view label, bool defaultOpen = true);
	ATHENA_API void TreePop();

	ATHENA_API bool BeginPropertyTable();
	ATHENA_API void EndPropertyTable();

	ATHENA_API void Property(std::string_view label, float height);
	ATHENA_API void Property(std::string_view label);


	ATHENA_API bool ComboBox(std::string_view label, const std::string_view* elems, uint32 elemsNum, std::string_view* selectedElem);
	ATHENA_API bool ComboBox(std::string_view label, const String* elems, uint32 elemsNum, String* selectedElem);
}
