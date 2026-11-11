TARGET := garden_guard
BUILD := build
# Override with: make BUTANO_ROOT=/path/to/butano
BUTANO_ROOT ?= $(HOME)/gba_development/butano
LIBBUTANO := $(BUTANO_ROOT)/butano
PYTHON := python3
SOURCES := src
INCLUDES := include
GRAPHICS := graphics $(BUTANO_ROOT)/common/graphics
DATA :=
AUDIO := audio
AUDIOBACKEND := maxmod
DMGAUDIO :=
DMGAUDIOBACKEND := null
ROMTITLE := GARDEN GUARD
ROMCODE := GRDN
USERFLAGS :=
USERCXXFLAGS := -Wall -Wextra -iquote $(BUTANO_ROOT)/common/include
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
