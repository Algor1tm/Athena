project "Box2D"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
	staticruntime "On"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    
    files
    {
        "src/collision/*.cpp",
        "src/collision/*.h",
        "src/common/*.cpp",
        "src/common/*.h",
        "src/dynamics/*.cpp",
        "src/dynamics/*.h",
        "src/rope/*.cpp",
        "src/rope/*.h",
        "include/box2d/*.h"
    }

    includedirs
	{
		"include",
		"src"
	}

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
	    runtime "Debug"
	    symbols "On"

	filter "configurations:Release"
		runtime "Release"
		optimize "On"
