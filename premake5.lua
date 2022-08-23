include "Dependencies.lua"

workspace "Athena"
	architecture "x86_64"
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

group "Dependencies"
	include "Athena/vendor/glad"
	include "Athena/vendor/GLFW"
	include "Athena/vendor/ImGui"
	include "Athena/vendor/spdlog"

group ""


include "Athena"
include "SandBox"
include "Athena-Editor"
