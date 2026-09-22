# Builds every example under examples/. Each one is an independent CE C
# Toolchain program compiled together with the library sources in src/; see
# project.mk for the actual build rules, and `make demo` / `make hello` to
# build just one.

EXAMPLES := demo hello

.PHONY: all clean $(EXAMPLES) $(addsuffix .clean,$(EXAMPLES))

all: $(EXAMPLES)

$(EXAMPLES):
	$(MAKE) -f project.mk EXAMPLE=$@

clean: $(addsuffix .clean,$(EXAMPLES))

%.clean:
	$(MAKE) -f project.mk EXAMPLE=$* clean
