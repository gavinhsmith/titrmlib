# Builds every example under examples/. Each one is an independent CE C
# Toolchain program compiled together with the library sources in src/; see
# project.mk for the actual build rules, and `make demo` / `make hello` to
# build just one.
#
# The hardware tests under tests/hw/ are built the same way (`make hw-build`)
# and run in CEmu's autotester with `make hw-test`; see tests/hw/run.py.

EXAMPLES := demo hello
HWTESTS := $(addprefix hw_,$(notdir $(patsubst %/,%,$(dir $(wildcard tests/hw/*/autotest.json)))))

PYTHON ?= python

.PHONY: all clean hw-build hw-test hw-record $(EXAMPLES) $(HWTESTS) $(addsuffix .clean,$(EXAMPLES) $(HWTESTS))

all: $(EXAMPLES)

$(EXAMPLES) $(HWTESTS):
	$(MAKE) -f project.mk EXAMPLE=$@

hw-build: $(HWTESTS)

# Builds and runs the hardware tests. Needs AUTOTESTER_ROM; HW_ARGS is passed
# to run.py, e.g. `make hw-test HW_ARGS="layout widgets"`.
hw-test:
	$(PYTHON) tests/hw/run.py $(HW_ARGS)

# Re-records the expected screen CRCs of failing hashes (review the PNGs!).
hw-record:
	$(PYTHON) tests/hw/run.py --record $(HW_ARGS)

clean: $(addsuffix .clean,$(EXAMPLES) $(HWTESTS))
	$(PYTHON) -c "import shutil; shutil.rmtree('tests/hw/build', ignore_errors=True)"

%.clean:
	$(MAKE) -f project.mk EXAMPLE=$* clean
