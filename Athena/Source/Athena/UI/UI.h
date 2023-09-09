#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Math/Vector.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Texture.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>

#include <string_view>


namespace Athena::UI
{
	enum class Fonts
	{
		Default = 0,
		Bold = 1,
		Default22 = 2,
		Default16 = Default
	};

	ATHENA_API void PushFont(Fonts font);
	ATHENA_API void PopFont();

	ATHENA_API void ShiftCursorX(float offset);
	ATHENA_API void ShiftCursorY(float offset);
	ATHENA_API void ShiftCursor(float offsetX, float offsetY);

	ATHENA_API ImColor MultiplyColorByScalar(const ImColor& color, float scalar);


	ATHENA_API bool TextInput(const String& label, String& destination, ImGuiInputTextFlags flags = 0);
	ATHENA_API bool TextInputWithHint(const std::string_view hint, String& destination, ImGuiInputTextFlags flags = 0);


	ATHENA_API bool TreeNode(std::string_view label, bool defaultOpen = true);
	ATHENA_API void TreePop();

	ATHENA_API bool BeginPropertyTable();
	ATHENA_API void EndPropertyTable();

	ATHENA_API void Property(std::string_view label, float height);
	ATHENA_API void Property(std::string_view label);
	ATHENA_API void PropertyImage(std::string_view label);


	ATHENA_API bool ComboBox(std::string_view label, const std::string_view* elems, uint32 elemsNum, std::string_view* selectedElem);
	ATHENA_API bool ComboBox(std::string_view label, const String* elems, uint32 elemsNum, String* selectedElem);

	ATHENA_API void ButtonImage(const Ref<Texture>& imageNormal, const Ref<Texture>& imageHovered, const Ref<Texture>& imagePressed,
		ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
		ImVec2 rectMin, ImVec2 rectMax);
	ATHENA_API void ButtonImage(const Ref<Texture>& image, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed);

	ATHENA_API void Image(Ref<Texture2D> image, const ImVec2& size, const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));

	ATHENA_API bool BeginMenubar(const ImRect& barRectangle);
	ATHENA_API void EndMenubar();

	ATHENA_API bool ButtonCentered(std::string_view label, const ImVec2& size = ImVec2(0, 0));
	ATHENA_API void TextCentered(std::string_view label);

	ATHENA_API void InvisibleItem(std::string_view id, ImVec2 size);
}
