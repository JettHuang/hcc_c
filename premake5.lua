-- premake5.lua project solution script

cfg_systemversion = "latest" -- "10.0.17763.0"   -- To use the latest version of the SDK available

-- solution
workspace "hcc"
    configurations { "Debug", "Release" }
    platforms { "Win32", "Win64" }

    location "build"
    defines { "_CRT_SECURE_NO_WARNINGS" }
    systemversion(cfg_systemversion)
	
    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
        targetsuffix("_d")

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"    

    filter "platforms:Win32"
        architecture "x32"

    filter "platforms:Win64"
        architecture "x64"

    filter {}

    targetdir("build/bin")  
    objdir("build/obj/%{prj.name}/%{cfg.buildcfg}")
	debugdir ("build/..");
	
-- project basic
project "basic"
    language "C"
    kind "StaticLib"

    includedirs {
        "./src/basic"
    }
	
    files {
		"./src/basic/*.h",
		"./src/basic/*.c",
		"./src/basic/*.inl"
    }

-- project libcpp
project "libcpp"
    language "C"
    kind "StaticLib"

	dependson { "basic" }
	links { "basic" }
	
    includedirs {
		"./src/basic",
        "./src/libcpp"
    }
	
    files {
		"./src/libcpp/*.h",
		"./src/libcpp/*.c"
    }

-- project libcc
project "libcc"
    language "C"
    kind "StaticLib"

	dependson { "basic" }
	links { "basic" }
	
    includedirs {
		"./src/basic",
        "./src/libcc"
    }
	
    files {
		"./src/libcc/*.h",
		"./src/libcc/*.c",
		"./src/libcc/lexer/*.h",
		"./src/libcc/lexer/*.c",
		"./src/libcc/parser/*.h",
		"./src/libcc/parser/*.c",
		"./src/libcc/generator/*.h",
		"./src/libcc/generator/*.c"
    }
	
group "tools"
-- project hcpp
project "hcpp"
    language "C"
    kind "ConsoleApp"

	dependson { "basic", "libcpp" }
	links { "basic", "libcpp" }
	
	debugargs { "-o \"test0.i\" dist/samples/test0.c" }
	
    includedirs {
		"./src/basic",
		"./src/libcpp",
		"./src/tools/common",
        "./src/tools/hcpp"
    }
	
    files {
		"./src/tools/common/*.h",
		"./src/tools/common/*.c",
		"./src/tools/hcpp/*.h",
		"./src/tools/hcpp/*.c"
    }

-- project hcc
project "hcc"
    language "C"
    kind "ConsoleApp"

	dependson { "basic", "libcc" }
	links { "basic", "libcc" }
	
	debugargs { "dist/samples/types.c" }
	
    includedirs {
		"./src/basic",
		"./src/libcc",
		"./src/tools/common",
        "./src/tools/hcc"
    }
	
    files {
		"./src/tools/common/*.h",
		"./src/tools/common/*.c",
		"./src/tools/hcc/*.h",
		"./src/tools/hcc/*.c"
    }

