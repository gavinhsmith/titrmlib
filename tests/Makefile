# Host-side unit tests: builds the library with the native C compiler against
# the stand-in CE headers in stubs/ and runs the suite.
#
#     make -C tests          # build and run
#     make -C tests CC=clang # any C99 compiler works
#
# Sanitizers are on by default; SANITIZE= turns them off (e.g. for compilers
# or platforms without ASan/UBSan).

CC ?= cc
SANITIZE ?= -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
CFLAGS ?= -std=c99 -g -O1 -Wall -Wextra -Werror
CPPFLAGS += -Istubs -I../src

BUILD := build
TEST := $(BUILD)/test_titrm

# titrm.c is #included by the test itself so it can read the cell grid.
SOURCES := test_titrm.c stubs/stubs.c ../src/titrm_widgets.c ../src/titrm_font.c
DEPS := $(SOURCES) ../src/titrm.c $(wildcard ../src/*.h stubs/*.h stubs/ti/*.h)

.PHONY: all test clean

all: test

test: $(TEST)
	./$(TEST)

$(TEST): $(DEPS)
	@mkdir -p $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZE) -o $@ $(SOURCES)

clean:
	rm -rf $(BUILD)
