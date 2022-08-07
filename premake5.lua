workspace "Athena"
	architecture "x64"
	startproject "Athena-Editor"

	configurations
	{ 
		"Debug",
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}


outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["glad"] = "Athena/vendor/glad/glad/include"
IncludeDir["GLFW"] = "Athena/vendor/GLFW/GLFW/include"
IncludeDir["ImGui"] = "Athena/vendor/ImGui/ImGui"
IncludeDir["spdlog"] = "Athena/vendor/spdlog/spdlog/include"
IncludeDir["stb_image"] = "Athena/vendor/stb_image"

group "Dependencies"
	include "Athena/vendor/glad"
	include "Athena/vendor/GLFW"
	include "Athena/vendor/ImGui"
	include "Athena/vendor/spdlog"

group ""

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
		"%{prj.name}/vendor/stb_image/stb_image.h",
		"%{prj.name}/vendor/stb_image/stb_image.cpp",
	}

	includedirs 
	{
		"%{prj.name}/src",
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
	

project "Athena-Editor"
	location "Athena-Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Athena/src",
		"%{IncludeDir.glad}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.stb_image}"
	}

	links 
	{
		"Athena"
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


project "SandBox"
	location "SandBox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Athena/src",
		"%{IncludeDir.glad}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.stb_image}"
	}

	links 
	{
		"Athena"
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
