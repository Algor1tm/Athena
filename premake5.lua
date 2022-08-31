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
		"ATN_SIMD",
		"YAML_CPP_STATIC_DEFINE"
	}


outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "Athena/vendor/glad"
	include "Athena/vendor/GLFW"
	include "Athena/vendor/ImGui"
	include "Athena/vendor/spdlog"
	include "Athena/vendor/yaml-cpp"
	include "Athena/vendor/Box2d"

group ""


include "Athena"
include "SandBox"
include "Athena-Editor"
