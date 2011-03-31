solution "Warlords"
	configurations { "Debug", "DebugOpt", "Release", "Retail" }
--	platforms { "Native", "x32", "x64", "Xbox360", "PS3" }
--	platforms { "Native", "Xbox" }
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

		dofile "../../Fuji/Project/fujiconfig.lua"