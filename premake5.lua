---@diagnostic disable: undefined-global, undefined-field
require"ecc/ecc"

workspace("CPong")
    kind("WindowedApp")
    configurations({ "Debug", "Release" })
    location("build")
    system("Windows")
    architecture("x86_64")

    project("CPong")
        language("C")
        cdialect("C99")
        targetdir("bin/Win64_%{cfg.buildcfg}")
        objdir("obj/Win64_%{cfg.buildcfg}")
        includedirs({ "./include/", })
        files({ "./src/*", "./include/*" })

filter("configurations:Debug")
    defines{"DEBUG"}
    staticruntime("off")
    runtime("Debug")
    symbols("On")
    ignoredefaultlibraries({ "MSVCRT" })

filter("configurations:Release")
    staticruntime("off")
    runtime("Release")
    symbols("Off")
    optimize("Speed")
