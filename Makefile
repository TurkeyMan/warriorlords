TARGETNAME = Warlords
TARGET = bin/$(TARGETNAME)

OBJS = Source/Warlords.o \
Source/Tileset.o \
Source/Map.o

ifeq ($(FUJIPATH),)
FUJIPATH = ../Fuji
endif

include $(FUJIPATH)/MakeGame.inc
