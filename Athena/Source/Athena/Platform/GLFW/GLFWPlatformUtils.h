#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/PlatformUtils.h"
#include "Athena/Core/Application.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <commdlg.h>


namespace Athena
{
	FilePath FileDialogs::OpenFile(std::string_view filter)
	{
#ifdef ATN_PLATFORM_WINDOWS
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter.data();
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return {};
#else
		ATN_CORE_ERROR("Athena does not support 'OpenFile(std::string_view filter)' function on current platform");
		return{};
#endif
	}

	FilePath FileDialogs::SaveFile(std::string_view filter)
	{
#ifdef ATN_PLATFORM_WINDOWS 
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter.data();
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		if (GetSaveFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return {};
#else
		ATN_CORE_ERROR("Athena does not support 'SaveFile(std::string_view filter)' function on current platform");
		return{};
#endif
	}
}
