
function ReasonableDefaults()
    local buildPath = path.join("build", _ACTION);
    local binPath = path.join(buildPath, "bin");
    local intermediatesPath = path.join(buildPath, "intermediates");
    local libsPath = path.join(intermediatesPath, "libs", "%{cfg.buildcfg}", "%{cfg.architecture}")
    local objPath = path.join(intermediatesPath, "obj")
    local projectsPath = path.join(buildPath, "projects")
    location(buildPath)

    --Change the location of projects and rules
    premake.override(_G, "project", function(base, ...)
        base(...);
        location(projectsPath)

        --Set up vpaths
        vpaths {
            ["include"] = "include/**.h",
            ["headers"] = "src/**.h",
            ["src"] = { "src/**.cpp", "src/**.c", "src/**.cxx" },
        }
    end);
    premake.override(_G, "rule", function(base, ...)
        base(...);
        location(path.join(buildPath, "projects"))
    end);

    --Change the location of intermediate files
    objdir(objPath)
    filter { "kind:*App" }
        targetdir(binPath)
        debugdir(binPath)
    filter { "kind:*App", "configurations:Debug" }
        targetsuffix "_d"
    filter { "kind:*App", "architecture:x86", "configurations:Debug" }
        targetsuffix "32_d"
    filter { "kind:*App", "architecture:x86", "configurations:Release" }
        targetsuffix "32"
    filter { "kind:StaticLib" }
        targetdir(libsPath)
    filter { "kind:SharedLib" }
        targetdir(binPath)
    filter { "kind:SharedLib", "configurations:Debug" }
        targetsuffix "_d"
    filter { "kind:SharedLib", "architecture:x86", "configurations:Debug" }
        targetsuffix "32_d"
    filter { "kind:SharedLib", "architecture:x86", "configurations:Release" }
        targetsuffix "32"

    --Set up standard build options
    filter { "configurations:Debug" }
        symbols "On"
        optimize "Off"
    filter { "configurations:Release" }
        symbols "Off"
        optimize "On"

    filter {}
end
