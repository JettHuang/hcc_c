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
		"./src/libcc/ir/*.h",
		"./src/libcc/ir/*.c",
		"./src/libcc/gen/*.h",
		"./src/libcc/gen/*.c",
		"./src/libcc/gen/x86/*.h",
		"./src/libcc/gen/x86/*.c",
		"./src/libcc/gen/x86/*.inl"
    }
	
group "tools"
-- project hcpp
project "hcpp"
    language "C"
    kind "ConsoleApp"

	dependson { "basic", "libcpp" }
	links { "basic", "libcpp" }
	
	debugargs { "-I dist/include -o dist/examples/helloworld_01/helloworld.i dist/examples/helloworld_01/helloworld.c" }
	
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
	
	postbuildcommands {  
		-- Copy the hcpp to the dist/bin folder.   
		'{COPY} "%{cfg.buildtarget.abspath}" "../dist/bin"',  
	}

-- project hcc
project "hcc"
    language "C"
    kind "ConsoleApp"

	dependson { "basic", "libcc" }
	links { "basic", "libcc" }
	
	debugargs { "-o dist/examples/helloworld_01/helloworld.asm dist/examples/helloworld_01/helloworld.i" }
	
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

	postbuildcommands {  
		-- Copy the hcc to the dist/bin folder.   
		'{COPY} "%{cfg.buildtarget.abspath}" "../dist/bin"',  
	}
	
