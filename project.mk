# CE C Toolchain project for one example program. Not meant to be run
# directly: the root Makefile invokes this once per example, e.g.
#
#     make -f project.mk EXAMPLE=demo
#
# The library in src/ is compiled straight into each example. The project
# lives at the repo root so no source path needs a `..` -- the toolchain
# cannot build those on Windows (it maps `..` to a directory named `_..`,
# and Windows drops the trailing dots).

EXAMPLE ?= demo

ifeq ($(EXAMPLE),demo)
NAME = TITRMDEM
DESCRIPTION = "titrmlib demo"
else ifeq ($(EXAMPLE),hello)
NAME = TITRMHEL
DESCRIPTION = "titrmlib hello"
else
$(error unknown EXAMPLE '$(EXAMPLE)': expected a directory under examples/ listed here)
endif

ICON =
COMPRESSED = NO

CFLAGS = -Wall -Wextra -Oz -Isrc
CXXFLAGS = -Wall -Wextra -Oz -Isrc

# src/*.c (the library) is picked up automatically as SRCDIR; add the example.
EXTRA_C_SOURCES = examples/$(EXAMPLE)/main.c
EXTRA_HEADERS = $(wildcard src/*.h)

OBJDIR = obj/$(EXAMPLE)
BINDIR = bin/$(EXAMPLE)

include $(shell cedev-config --makefile)
