workspace "Athena"
	architecture "x64"

	configurations
	{ 
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Athena"
	location "Athena"	
	kind "SharedLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir    ("bin-ints/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs 
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include" 
	}

	pchheader  "atnpch.h"
	pchsource  "Athena/src/atnpch.cpp"

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines 
		{
			"ATN_PLATFORM_WINDOWS",
			"ATN_BUILD_DLL"
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/SandBox")
		}


	filter "configurations:Debug"
		defines "ATN_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "ATN_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "ATN_DIST"
		optimize "On"
	

project "SandBox"
	location "SandBox"
	kind "ConsoleApp"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-ints/" .. outputdir .. "/%{prj.name}")

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
		symbols "On"

	filter "configurations:Release"
		defines "ATN_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "ATN_DIST"
		optimize "On"
