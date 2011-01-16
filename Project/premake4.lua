solution "Warlords"
	configurations { "Debug", "DebugOpt", "Release", "Retail" }
--	platforms { "Native", "x32", "x64", "Xbox360", "PS3" }
	platforms { "Native", "x32", "x64" }

	-- include the fuji project...
	dofile  "../../Fuji/Project/fujiproj.lua"

	project "Warlords"
		kind "WindowedApp"
		language "C++"
		files { "../Source/**.h", "../Source/**.cpp" }
		files { "../Data/UI/**.ini" }

		includedirs { "../Source/" }
		objdir "../Build/"
		targetdir "../"

		flags { "StaticRuntime", "NoExceptions", "NoRTTI" }

--		pchheader "Warlords.h"
--		pchsource "Warlords.cpp"

		links { "Fuji" }

		configuration "Debug"
			defines { "DEBUG", "_DEBUG" }
			flags { "Symbols" }
			targetsuffix "_Debug"

		configuration "DebugOpt"
			defines { "DEBUG", "_DEBUG" }
			flags { "Symbols", "OptimizeSpeed" }
			targetsuffix "_DebugOpt"

		configuration "Release"
			defines { "NDEBUG", "_RELEASE" }
			flags { "OptimizeSpeed" }
			targetsuffix "_Release"

		configuration "Retail"
			defines { "NDEBUG", "_RETAIL" }
			flags { "OptimizeSpeed" }
			targetsuffix "_Retail"


		-- need to filter the proper set of macros for various visual studio platforms
		configuration { "windows or Xbox360", "not PS3" }
			defines { "WIN32" }
		configuration { "windows", "not Xbox360", "not PS3" }
			defines { "_WINDOWS" }
		configuration "Xbox360"
			defines { "_XBOX" }
		configuration "PS3"
			defines { "_PS3" }
