project "spdlog"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
	staticruntime "On"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")


	files
	{
		"spdlog/include/spdlog/**.h",
		"spdlog/src/**.cpp"
	}

	defines
	{
		"SPDLOG_COMPILED_LIB",
		"_CRT_SECURE_NO_WARNINGS"
	}

	includedirs
	{
		"spdlog/include"
	}

	filter "system:windows"
		systemversion "latest"

    filter "configurations:Debug"
	    runtime "Debug"
	    symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
