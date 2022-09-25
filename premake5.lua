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
	include "Athena/ThirdParty/Box2D"
	include "Athena/ThirdParty/glad"
	include "Athena/ThirdParty/GLFW"
	include "Athena/ThirdParty/ImGui"
	include "Athena/ThirdParty/spdlog"
	include "Athena/ThirdParty/yaml-cpp"

group ""


include "Athena"
include "SandBox"
include "Athena-Editor"
