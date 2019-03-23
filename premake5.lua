require("reasonable_defaults")
ReasonableDefaults()

workspace "LinearFitSearch"
    configurations { "Debug", "Release" }
    platforms { "x86_64" }
    characterset "MBCS"
    cppdialect "C++14"
    rtti "Off"
    exceptionhandling "Off"
    startproject "LinearFitSearch"
    flags {
        "FatalWarnings",
    }

	project "LinearFitSearch"
		kind "ConsoleApp"
		files { "main.cpp" }

        filter { "system:linux" }
            links { "pthread" }
        filter {}
