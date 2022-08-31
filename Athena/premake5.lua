project "Athena"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "On"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir    ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader  "atnpch.h"
	pchsource  "src/atnpch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp",
		"vendor/entt/entt.h",
		"vendor/ImGuizmo/ImGuizmo.h",
		"vendor/ImGuizmo/ImGuizmo.cpp",
		"vendor/stb_image/stb_image.h",
		"vendor/stb_image/stb_image.cpp"
	}

	includedirs 
	{
		"src",
		"vendor",
		"%{IncludeDir.Box2d}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.glad}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.ImGui}",
		"vendor/ImGui/ImGui",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.yaml}"
	}
	
	links
	{
		"Box2d",
		"glad",
		"GLFW",
		"opengl32.lib",
		"ImGui",
		"spdlog",
		"yaml-cpp"
	}

	defines 
	{
		"_CRT_SECURE_NO_WARNINGS",
		"ATN_BUILD_DLL",
		"GLFW_INCLUDE_NONE",
	}

	filter "files:vendor/ImGuizmo/**.cpp"
	flags { "NoPCH" }

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
	