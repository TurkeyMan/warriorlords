CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(Warlords)

SET(Warlords_VERSION_MAJOR 1)
SET(Warlords_VERSION_MINOR 0)

SET(FUJI_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../Fuji)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/Source ${FUJI_PATH}/Include ${FUJI_PATH}/Middleware)


# ADD_SUBDIRECTORY(Source)

SET(source_files
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screens/Battle.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screens/CastleConfig.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screens/GroupConfig.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screens/HomeScreen.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screens/Inventory.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screens/JoinGameScreen.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screens/LobbyScreen.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screens/LoginScreen.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screens/MapScreen.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screens/MenuScreen.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screens/MiniMap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screens/RequestBox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screens/UnitConfig.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screens/Window.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Tools/ResourceCache.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/Action.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/ButtonProp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/Entity.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/ImageProp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/LayoutProp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/ListProp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/ModelProp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/SelectBox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/StringProp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/TextProp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Button.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Display.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Editor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Game.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/GameData.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/HTTP.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/InputHandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/ListBox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Map.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Path.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Screen.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/SelectBox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/ServerRequest.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Session.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/StringBox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/StringEntryLogic.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Tileset.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Unit.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Warlords.cpp
)

ADD_EXECUTABLE(Warlords ${source_files})
TARGET_LINK_LIBRARIES(Warlords ${FUJI_PATH}/Lib/libFuji_PC_Debug)
