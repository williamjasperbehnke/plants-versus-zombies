TARGET := plants_vs_zombies
BUILD := build
# Override with: make BUTANO_ROOT=/path/to/butano
BUTANO_ROOT ?= $(HOME)/gba_development/butano
LIBBUTANO := $(BUTANO_ROOT)/butano
PYTHON := python3
SOURCES := src
INCLUDES := include
GRAPHICS := $(wildcard graphics/*/) $(BUTANO_ROOT)/common/graphics
DATA :=
AUDIO := audio
AUDIOBACKEND := maxmod
DMGAUDIO :=
DMGAUDIOBACKEND := null
ROMTITLE := PLANTSVSZOMB
ROMCODE := GRDN
# Lane depth, HUD layers, and the reward overlay require more than the default 16 sort layers.
USERFLAGS := -DBN_CFG_SPRITES_MAX_SORT_LAYERS=64
USERCXXFLAGS := -Wall -Wextra -iquote $(BUTANO_ROOT)/common/include
# Debug progress lives in a separate ROM, object directory, and SRAM region.
DEBUG_SAVE ?= 0
ifeq ($(DEBUG_SAVE),1)
TARGET := plants_vs_zombies_debug
BUILD := build_debug
ROMTITLE := PVZ DEBUG
USERCXXFLAGS += -DGARDEN_DEBUG_SAVE
else ifneq ($(DEBUG_SAVE),0)
$(error DEBUG_SAVE must be 0 or 1)
endif
USERASFLAGS :=
USERLDFLAGS :=
USERLIBDIRS :=
USERLIBS :=
DEFAULTLIBS :=
STACKTRACE :=
USERBUILD :=
EXTTOOL :=
export LIBBUTANOABS := $(realpath $(LIBBUTANO))
ifeq ($(LIBBUTANOABS),)
$(error Butano not found. Set BUTANO_ROOT to the Butano repository directory)
endif
include $(LIBBUTANOABS)/butano.mak
