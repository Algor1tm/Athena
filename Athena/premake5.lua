project "Athena"
	kind "SharedLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "Off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir    ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchsource  "Source/atnpch.cpp"
	pchheader  "atnpch.h"

	files
	{
		"Source/**.h",
		"Source/**.cpp",
		"ThirdParty/entt/entt.h",
		"ThirdParty/ImGuizmo/ImGuizmo.h",
		"ThirdParty/ImGuizmo/ImGuizmo.cpp",
		"ThirdParty/stb_image/stb_image.h",
		"ThirdParty/stb_image/stb_image.cpp"
	}

	includedirs 
	{
		"Source",
		"ThirdParty",
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.glad}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.yaml}",
	}
	
	links
	{
		"Box2D",
		"glad",
		"GLFW",
		"ImGui",
		"spdlog",
		"yaml-cpp",
	}

	defines 
	{
		"_CRT_SECURE_NO_WARNINGS",
		"ATN_BUILD_DLL",
		"GLFW_INCLUDE_NONE",
	}

	postbuildcommands
	{
		("{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/Athena-Editor/\""),
		("{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/SandBox/\"")
	}

	filter "files:ThirdParty/ImGuizmo/**.cpp"
		flags { "NoPCH" }

	filter "system:windows"
		staticruntime "On"
		systemversion "latest"

		disablewarnings 
		{
			"4251"
		}

		files
		{
			"Athena.def"
		}

		links
		{
			"d3d11.lib",
			"DXGI.lib"
		}

	filter "system:linux"
		pic "On"
		systemversion "latest"

		visibility "Default"

		links
		{
			"opengl32.lib"
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
	