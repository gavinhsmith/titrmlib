# CE C Toolchain project for one example program or hardware test. Not meant
# to be run directly: the root Makefile invokes this once per program, e.g.
#
#     make -f project.mk EXAMPLE=demo
#     make -f project.mk EXAMPLE=hw_layout   # tests/hw/layout/main.c
#
# The library in src/ is compiled straight into each example. The project
# lives at the repo root so no source path needs a `..` -- the toolchain
# cannot build those on Windows (it maps `..` to a directory named `_..`,
# and Windows drops the trailing dots).

EXAMPLE ?= demo

# Program names for the hardware tests; each must match the "target" name in
# tests/hw/<test>/autotest.json.
HW_NAME_canary = TTCANARY
HW_NAME_glyphs = TTGLYPHS
HW_NAME_layout = TTLAYOUT
HW_NAME_widgets = TTWIDGET
HW_NAME_ticks = TTTICKS
HW_NAME_selfcheck = TTSELFCK
HW_NAME_perf = TTPERF
HW_NAME_scenes = TTSCENES

ifeq ($(EXAMPLE),demo)
NAME = TITRMDEM
DESCRIPTION = "titrmlib demo"
else ifeq ($(EXAMPLE),hello)
NAME = TITRMHEL
DESCRIPTION = "titrmlib hello"
else ifneq ($(HW_NAME_$(EXAMPLE:hw_%=%)),)
NAME = $(HW_NAME_$(EXAMPLE:hw_%=%))
DESCRIPTION = "titrmlib hw test"
MAIN = tests/hw/$(EXAMPLE:hw_%=%)/main.c
else
$(error unknown EXAMPLE '$(EXAMPLE)': expected a directory under examples/ or tests/hw/ listed here)
endif

MAIN ?= examples/$(EXAMPLE)/main.c

ICON =
COMPRESSED = NO

CFLAGS = -Wall -Wextra -Oz -Isrc
CXXFLAGS = -Wall -Wextra -Oz -Isrc

# src/*.c (the library) is picked up automatically as SRCDIR; add the program.
EXTRA_C_SOURCES = $(MAIN)
EXTRA_HEADERS = $(wildcard src/*.h)

OBJDIR = obj/$(EXAMPLE)
BINDIR = bin/$(EXAMPLE)

include $(shell cedev-config --makefile)
