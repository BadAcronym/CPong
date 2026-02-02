---@diagnostic disable: undefined-global, undefined-field
workspace("CPong")
    configurations({ "Debug", "Release" })
    location("build")
    system("Windows")
    architecture("x86_64")

    project("CPong")
        language("C")
        cdialect("C99")
        targetdir("bin/%{cfg.platform}_%{cfg.buildcfg}")
        objdir("obj/%{cfg.platform}_%{cfg.buildcfg}")
        includedirs({
            "./include/",
        })
        files({ "./src/*", "./include/*" })

filter("configurations:Debug")
    kind("ConsoleApp")
    defines{"DEBUG"}
    staticruntime("off")
    runtime("Debug")
    symbols("On")
    ignoredefaultlibraries({ "MSVCRT" })

filter("configurations:Release")
    kind("WindowedApp")
    staticruntime("off")
    runtime("Release")
    symbols("Off")
    optimize("Speed")
