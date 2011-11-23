solution "Warlords"
	configurations { "Debug", "DebugOpt", "Release", "Retail" }
--	platforms { "Native", "x32", "x64", "Xbox360", "PS3" }
--	platforms { "Native", "Xbox" }
--	platforms { "Native", "x32", "x64", "Android" }
	platforms { "Native", "x32", "x64" }

	-- include the fuji project...
	dofile  "../../Fuji/Fuji/Private/Project/fujiproj.lua"

	-- include the Haku project...
	dofile "../../Fuji/Haku/Project/hakuproj.lua"

	project "Warlords"
		kind "WindowedApp"
		language "C++"
		files { "../Source/**.h", "../Source/**.cpp" }
		files { "../Data/UI/**.ini", "../Data/UI/**.xml" }

		includedirs { "../Source/" }
		objdir "../Build/"
		targetdir "../"
		debugdir "../"

		flags { "StaticRuntime", "NoExceptions", "NoRTTI", "WinMain" }

--		pchheader "Warlords.h"
--		pchsource "Warlords.cpp"

		links { "Fuji", "Haku" }

		dofile "../../Fuji/Fuji/Public/Project/fujiconfig.lua"
		dofile "../../Fuji/Haku/Project/hakuconfig.lua"
