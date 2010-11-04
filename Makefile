TARGETNAME = Warlords
TARGET = bin/$(TARGETNAME)

OBJS = Source/Button.o \
Source/Display.o \
Source/Game.o \
Source/InputHandler.o \
Source/Map.o \
Source/Screen.o \
Source/ServerRequest.o \
Source/StringBox.o \
Source/Tileset.o \
Source/Warlords.o \
Source/Editor.o \
Source/HTTP.o \
Source/ListBox.o \
Source/Path.o \
Source/SelectBox.o \
Source/Session.o \
Source/StringEntryLogic.o \
Source/Unit.o \
Source/Screens/Battle.o \
Source/Screens/GroupConfig.o \
Source/Screens/Inventory.o \
Source/Screens/LobbyScreen.o \
Source/Screens/MapScreen.o \
Source/Screens/MiniMap.o \
Source/Screens/UnitConfig.o \
Source/Screens/CastleConfig.o \
Source/Screens/HomeScreen.o \
Source/Screens/JoinGameScreen.o \
Source/Screens/LoginScreen.o \
Source/Screens/MenuScreen.o \
Source/Screens/RequestBox.o \
Source/Screens/Window.o

ifeq ($(FUJIPATH),)
FUJIPATH = ../Fuji
endif

include $(FUJIPATH)/MakeGame.inc
