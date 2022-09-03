project "SandBox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "On"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"src",
		"%{wks.location}/Athena/src",
		"%{wks.location}/Athena/vendor",
		"%{IncludeDir.entt}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.spdlog}"
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
