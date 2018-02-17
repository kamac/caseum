workspace "main"
    configurations { "Debug", "Release" }
    platforms { "Win32", "Win64" }
    location "build"

    filter "configurations:Debug"
        defines { "DEBUG" }

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter { "platforms:Win32" }
        system "Windows"
        architecture "x32"

    filter { "platforms:Win64" }
        system "Windows"
        architecture "x64"

project "caseum"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin"
    files { "./src/**.cpp", "./src/**.h" }