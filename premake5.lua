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

	defines
	{
		"ATN_SIMD"
	}

	filter "system:windows"
		buildoptions
		{
			"/arch:AVX2"
		}


outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["entt"] = "Athena/vendor/entt"
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

include "Athena"
include "Athena-Editor"
include "SandBox"
