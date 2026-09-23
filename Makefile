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

.PHONY: all clean hw-build hw-test hw-record $(EXAMPLES) $(HWTESTS)

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

# Removes all build output. Done here rather than with CEdev's clean, whose
# Windows version silently fails on paths with '/' in them.
clean:
	$(PYTHON) -c "import glob, shutil; [shutil.rmtree(d, ignore_errors=True) for d in glob.glob('bin/*/') + ['obj', 'tests/hw/build']]"
