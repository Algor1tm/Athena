#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/PlatformUtils.h"
#include "Athena/Core/Application.h"

#include <GLFW/glfw3.h>


#ifdef ATN_PLATFORM_WINDOWS

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <commdlg.h>
#include <intrin.h>

namespace Athena
{
	static DWORD CountSetBits(ULONG_PTR bitMask)
	{
		DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
		DWORD bitSetCount = 0;
		ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
		DWORD i;

		for (i = 0; i <= LSHIFT; ++i)
		{
			bitSetCount += ((bitMask & bitTest) ? 1 : 0);
			bitTest /= 2;
		}

		return bitSetCount;
	}

	void Platform::GetSystemInfo(SystemInfo* info)
	{
		typedef BOOL(WINAPI* LPFN_GLPI)(
			PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
			PDWORD);

		int cpuInfo[4] = { -1 };
		char cpuString[0x40];

		memset(cpuString, 0, sizeof(cpuString));

		__cpuid(cpuInfo, 0x80000002);
		memcpy(cpuString, cpuInfo, sizeof(cpuInfo));

		__cpuid(cpuInfo, 0x80000003);
		memcpy(cpuString + 16, cpuInfo, sizeof(cpuInfo));

		__cpuid(cpuInfo, 0x80000004);
		memcpy(cpuString + 32, cpuInfo, sizeof(cpuInfo));

		info->CPUBrandString = cpuString;

		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		GlobalMemoryStatusEx(&statex);

		info->TotalPhysicalMemoryKB = statex.ullTotalPhys / 1024;

		LPFN_GLPI glpi = (LPFN_GLPI)GetProcAddress(
			GetModuleHandle(TEXT("kernel32")),
			"GetLogicalProcessorInformation");

		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
		DWORD returnLength = 0;
		DWORD logicalProcessorCount = 0;
		DWORD processorCoreCount = 0;
		DWORD byteOffset = 0;


		while (true)
		{
			DWORD rc = glpi(buffer, &returnLength);
			if (rc != FALSE)
				break;

			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				if (buffer)
					free(buffer);

				buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
					returnLength);

				if (NULL == buffer)
				{
					ATN_CORE_ERROR_TAG("Platform", "GetSystemInfo allocation failure\n");
					return;
				}
			}
			else
			{
				ATN_CORE_ERROR_TAG_("Platform", "GetSystemInfo: {}", GetLastError());
				return;
			}
		}

		ptr = buffer;

		while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
		{
			if (ptr->Relationship == RelationProcessorCore)
			{
				processorCoreCount++;

				// A hyper threaded core supplies more than one logical processor.
				logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
			}
			byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
			ptr++;
		}

		info->NumberOfProcessorCores = processorCoreCount;
		info->NumberOfLogicalProcessors = logicalProcessorCount;
	}

	double Platform::GetHighPrecisionTime()
	{
		static double frequency = 0.0;
		if (frequency == 0.0)
		{
			LARGE_INTEGER li;
			QueryPerformanceFrequency(&li);
			frequency = (double)li.QuadPart / 1000.0;
		}

		LARGE_INTEGER result;
		QueryPerformanceCounter(&result);

		return double(result.QuadPart) / frequency;
	}

	FilePath FileDialogs::OpenFile(std::string_view filter)
	{
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
	}

	FilePath FileDialogs::SaveFile(std::string_view filter)
	{
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
	}
}

#else
namespace Athena
{
	void Platform::GetSystemInfo(SystemInfo* info)
	{
		ATN_CORE_ERROR("Athena does not support 'Platform::GetSystemInfo(SystemInfo* info)' function on current platform");
	}

	double Platform::GetHighPrecisionTime()
	{
		ATN_CORE_ERROR("Athena does not support 'Platform::GetHighPrecisionTime()' function on current platform");
	}

	FilePath FileDialogs::OpenFile(std::string_view filter)
	{
		ATN_CORE_ERROR("Athena does not support 'FileDialogs::OpenFile(std::string_view filter)' function on current platform");
		return{};
	}

	FilePath FileDialogs::SaveFile(std::string_view filter)
	{
		ATN_CORE_ERROR("Athena does not support 'FileDialogs::SaveFile(std::string_view filter)' function on current platform");
		return{};
	}
}
#endif
