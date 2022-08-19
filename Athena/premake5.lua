project "Athena"
	location "Athena"	
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "On"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader  "atnpch.h"
	pchsource  "Athena/src/atnpch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/entt/entt.h",
		"%{prj.name}/vendor/stb_image/stb_image.h",
		"%{prj.name}/vendor/stb_image/stb_image.cpp"
	}

	includedirs 
	{
		"%{prj.name}/src",
		"%{IncludeDir.entt}",
		"%{IncludeDir.glad}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.stb_image}"
	}
	
	links
	{
		"glad",
		"GLFW",
		"ImGui",
		"spdlog",
		"opengl32.lib"
	}

	defines 
	{
		"_CRT_SECURE_NO_WARNINGS",
		"ATN_BUILD_DLL",
		"GLFW_INCLUDE_NONE"
	}

	filter "system:windows"
		staticruntime "On"
		systemversion "latest"


	filter "configurations:Debug"
		defines "ATN_DEBUG"
	    runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "ATN_RELEASE"
		runtime "Release"
		optimize "On"

	filter "configurations:Dist"
		defines "ATN_DIST"
		runtime "Release"
		optimize "On"
