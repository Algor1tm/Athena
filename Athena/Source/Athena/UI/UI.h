#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Math/Vector.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/Image.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>

#include <string_view>


namespace Athena::UI
{
	// Fonts
	enum class Fonts
	{
		Default = 0,
		Bold = 1,
		Default22 = 2,
		Default16 = Default
	};

	ATHENA_API void* GetTextureID(const Ref<Texture2D>& texture);
	ATHENA_API void* GetTextureID(const Ref<Image>& image);

	ATHENA_API void PushFont(Fonts font);
	ATHENA_API void PopFont();

	// Misc
	ATHENA_API void ShiftCursorX(float offset);
	ATHENA_API void ShiftCursorY(float offset);
	ATHENA_API void ShiftCursor(float offsetX, float offsetY);

	ATHENA_API ImColor MultiplyColorByScalar(const ImColor& color, float scalar);

	ATHENA_API bool TextInput(const String& label, String& destination, ImGuiInputTextFlags flags = 0);
	ATHENA_API bool TextInputWithHint(const std::string_view hint, String& destination, ImGuiInputTextFlags flags = 0);

	// Tree
	ATHENA_API bool TreeNode(std::string_view label, bool defaultOpen = true);
	ATHENA_API void TreePop();

	// Properties
	ATHENA_API bool BeginPropertyTable();
	ATHENA_API void EndPropertyTable();

	ATHENA_API void PropertyRow(std::string_view label, float height);

	ATHENA_API bool PropertyDrag(std::string_view label, float* value, float speed = 1.f, float min = 0.f, float max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	ATHENA_API bool PropertyDrag(std::string_view label, Vector2* value, float speed = 1.f, float min = 0.f, float max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	ATHENA_API bool PropertyDrag(std::string_view label, Vector3* value, float speed = 1.f, float min = 0.f, float max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	ATHENA_API bool PropertyDrag(std::string_view label, Vector4* value, float speed = 1.f, float min = 0.f, float max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	ATHENA_API bool PropertyDrag(std::string_view label, int* value, float speed = 1.f, float min = 0.f, float max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	ATHENA_API bool PropertySlider(std::string_view label, float* value, float min, float max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	ATHENA_API bool PropertySlider(std::string_view label, Vector2* value, float min, float max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
	ATHENA_API bool PropertyColor3(std::string_view label, float color[3], ImGuiColorEditFlags flags = 0);
	ATHENA_API bool PropertyColor4(std::string_view label, float color[4], ImGuiColorEditFlags flags = 0);
	ATHENA_API bool PropertyColorU32(std::string_view label, ImU32* color, ImGuiColorEditFlags flags = 0);
	ATHENA_API bool PropertyCheckbox(std::string_view label, bool* value);
	ATHENA_API bool PropertyCombo(std::string_view label, const std::string_view* elems, uint32 elemsNum, std::string_view* selectedElem);
	ATHENA_API bool PropertyCombo(std::string_view label, const String* elems, uint32 elemsNum, String* selectedElem);
	ATHENA_API bool PropertyImage(std::string_view label, const Ref<Texture2D>& tex, ImVec2 size, float frame_padding = -1, const ImVec4& bg_col = { 0, 0, 0, 0 }, const ImVec4& tint_col = { 1, 1, 1, 1 });
	ATHENA_API void PropertyText(std::string_view label, std::string_view value);

	// Widgets
	ATHENA_API bool ComboBox(std::string_view label, const std::string_view* elems, uint32 elemsNum, std::string_view* selectedElem);
	ATHENA_API bool ComboBox(std::string_view label, const String* elems, uint32 elemsNum, String* selectedElem);

	ATHENA_API void ButtonImage(const Ref<Image>& imageNormal, const Ref<Image>& imageHovered, const Ref<Image>& imagePressed,
		ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
		ImVec2 rectMin, ImVec2 rectMax);
	ATHENA_API void ButtonImage(const Ref<Image>& image, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed);

	ATHENA_API void DrawImage(const Ref<Image>& image, const ImVec2& size, const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));

	ATHENA_API bool ButtonCentered(std::string_view label, const ImVec2& size = ImVec2(0, 0));
	ATHENA_API void TextCentered(std::string_view label);

	ATHENA_API void InvisibleItem(std::string_view id, ImVec2 size);

	// Menubar with custom rect
	ATHENA_API bool BeginMenubar(const ImRect& barRectangle);
	ATHENA_API void EndMenubar();
}
