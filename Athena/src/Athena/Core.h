#pragma once


#ifdef ATN_PLATFORM_WINDOWS
	#ifdef ATN_BUILD_DLL
		#define ATHENA_API __declspec(dllexport)
	#else
		#define  ATHENA_API __declspec(dllimport)
	#endif
#else
	#error Athena only supports Windows!
#endif
