# Fans `make` out to every test project under test/, each of which is an
# independent CE C Toolchain program that builds against the library
# sources in src/. See test/*/makefile for the actual build rules.

TESTS := $(patsubst %/makefile,%,$(wildcard test/*/makefile))

.PHONY: all clean $(TESTS) $(addsuffix .clean,$(TESTS))

all: $(TESTS)

$(TESTS):
	$(MAKE) -C $@

clean: $(addsuffix .clean,$(TESTS))

%.clean:
	$(MAKE) -C $* clean
