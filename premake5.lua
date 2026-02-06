workspace "Set5"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "A3"

project "A3"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"
    warnings "Extra"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files { "src/**.cpp", "src/**.h" }
    includedirs { "src" }
	
    filter "configurations:Debug"
        symbols "On"
        defines { "_DEBUG" }

    filter "configurations:Release"
        optimize "Speed"
        defines { "NDEBUG" }
		
	 filter "action:vs2022"
        toolset "v145"
