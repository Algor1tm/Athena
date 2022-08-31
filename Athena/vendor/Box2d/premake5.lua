project "Box2d"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
	staticruntime "On"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    
    files
    {
        "Box2d/src/collision/*.cpp",
        "Box2d/src/collision/*.h",
        "Box2d/src/common/*.cpp",
        "Box2d/src/common/*.h",
        "Box2d/src/dynamics/*.cpp",
        "Box2d/src/dynamics/*.h",
        "Box2d/src/rope/*.cpp",
        "Box2d/src/rope/*.h",
        "Box2d/include/box2d/*.h"
    }

    includedirs
	{
		"Box2d/include",
		"Box2d/src"
	}

    defines
    {
        "_CRT_SECURE_NO_WARNINGS"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
	    runtime "Debug"
	    symbols "On"

	filter "configurations:Release"
		runtime "Release"
		optimize "On"
