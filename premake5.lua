workspace "Athena"
	architecture "x64"
	startproject "SandBox"

	configurations
	{ 
		"Debug",
		"Release",
		"Dist"
	}


outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "Athena/vendor/GLFW/include"
IncludeDir["Glad"] = "Athena/vendor/Glad/include"
IncludeDir["ImGui"] = "Athena/vendor/ImGui"

group "Dependencies"
	include "Athena/vendor/GLFW"
	include "Athena/vendor/Glad"
	include "Athena/vendor/ImGui"

group ""

project "Athena"
	location "Athena"	
	kind "SharedLib"
	language "C++"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader  "atnpch.h"
	pchsource  "Athena/src/atnpch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs 
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}"
	}
	
	links
	{
		"GLFW",
		"Glad",
		"ImGui",
		"opengl32.lib"
	}

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"

		defines 
		{
			"ATN_PLATFORM_WINDOWS",
			"ATN_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

		postbuildcommands
		{
			("IF NOT EXIST ../bin/" .. outputdir .. "/SandBox mkdir ../bin/" .. outputdir .. "/SandBox/"),
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/SandBox")
		}


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
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Athena/vendor/spdlog/include",
		"Athena/src"
	}

	links 
	{
		"Athena"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines 
		{
			"ATN_PLATFORM_WINDOWS"
		}

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
