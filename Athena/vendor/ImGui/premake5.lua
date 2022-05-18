project "ImGui"
    kind "StaticLib"
    language "C++"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    
    files
    {
        "include/imconfig.h",
        "include/imgui.cpp",
        "include/imgui.h",
        "include/imgui_demo.cpp",
        "include/imgui_draw.cpp",
        "include/imgui_internal.h",
        "include/imgui_tables.cpp",
        "include/imgui_widgets.cpp",
        "include/imstb_rectpack.h",
        "include/imstb_textedit.h",
        "include/imstb_truetype.h",
    }

    includedirs
	{
		"include",
	}

    filter "system:windows"
        systemversion "latest"
        cppdialect "C++17"

    filter "configurations:Debug"
	    runtime "Debug"
	    symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"
        symbols "off"
