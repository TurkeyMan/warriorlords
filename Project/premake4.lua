solution "Warlords"
	configurations { "Debug", "DebugOpt", "Release", "Retail" }
--	platforms { "Native", "x32", "x64", "Xbox360", "PS3" }
--	platforms { "Native", "Xbox" }
	platforms { "Native", "x32", "x64", "Android" }

	-- include the fuji project...
	dofile  "../../Fuji/Fuji/Private/Project/fujiproj.lua"

	project "Warlords"
		kind "WindowedApp"
		language "C++"
		files { "../Source/**.h", "../Source/**.cpp" }
		files { "../Data/UI/**.ini" }

		includedirs { "../Source/" }
		objdir "../Build/"
		targetdir "../"

		flags { "StaticRuntime", "NoExceptions", "NoRTTI", "WinMain" }

--		pchheader "Warlords.h"
--		pchsource "Warlords.cpp"

		links { "Fuji" }

		dofile "../../Fuji/Fuji/Public/Project/fujiconfig.lua"
