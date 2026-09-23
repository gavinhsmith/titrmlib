---
---

# Contributing to titrmlib

## Setting up

You need:

- The [CE C/C++ Toolchain](https://github.com/CE-Programming/toolchain) (CEdev)
  with its `bin/` on `PATH`. It provides `make`, `cedev-config` and
  `cemu-autotester`.
- Python 3, for the font generator and the hardware test runner. Only the
  standard library is used.
- A host C compiler (gcc or clang) for the unit tests. On Windows,
  [MSYS2](https://www.msys2.org/) provides one: `pacman -S mingw-w64-ucrt-x86_64-gcc`.
  Put `C:\msys64\ucrt64\bin` **above** `C:\Program Files\Git\mingw64\bin` in
  your `PATH`. Git ships older copies of DLLs that gcc loads, and if Git's
  folder comes first, gcc fails without printing an error. Git Bash always puts
  its own folder first, so run the unit tests from PowerShell or cmd.
- For the API docs only: [Doxygen](https://www.doxygen.nl/) 1.18.0 on `PATH`
  (on Windows: `winget install DimitriVanHeesch.Doxygen`) and
  [Node.js](https://nodejs.org/), which runs moxygen through `npx`.
- For the hardware tests only: a TI-84 Plus CE ROM image (see
  [Hardware tests](#hardware-tests)).

## Repository layout

| Path | Contents |
|---|---|
| `src/` | The library. `titrm.h` is the whole public API |
| `src/FONT.md` | Character code → glyph → purpose table (generated) |
| `examples/hello/` | The smallest useful program |
| `examples/demo/` | A mock Wi-Fi manager using every feature: scenes, an overlay dialog with buttons, a log |
| `docs/` | API reference (generated) |
| `Doxyfile` | Doxygen settings for the API docs |
| `tools/gen_font.py` | Converts `tools/petabyt-font/font.h` into `src/titrm_font.c`, `src/titrm_chars.h` and `src/FONT.md` |
| `tests/` | Host unit tests and their stand-in CE headers (`tests/stubs/`) |
| `tests/hw/` | Hardware tests for CEmu's autotester, and their runner `run.py` |
| `project.mk` | CEdev build rules for one example or hardware test program |
| `bin/`, `obj/` | Build output |

## Building

```sh
make               # build every example into bin/<example>/
make demo          # or one of them
make hw-build      # build the hardware test programs
make clean         # remove all build output
```

The library is compiled straight into each program (see `project.mk`). Keep
every source file inside the repository root: on Windows, CEdev can't build
sources reached through `..`.

## Changing the font

Don't edit `src/titrm_font.c`, `src/titrm_chars.h` or `src/FONT.md` by hand.
Change `tools/gen_font.py` and regenerate:

```sh
python tools/gen_font.py
```

Commit the script and the regenerated files together. CI fails if they don't
match.

## API docs

`docs/` is generated from the doc comments in `src/titrm.h`.
[Doxygen](https://www.doxygen.nl/) reads the header into XML, and
[moxygen](https://github.com/sourcey/moxygen) turns the XML into one Markdown
page per group, plus the index `docs/api.md`.

GitHub Pages publishes the repository root from `main` as
<https://gavinhsmith.github.io/titrmlib/>, with `README.md` as the front page and
the API reference under `docs/`. Site settings are in `_config.yml`.

In the header:

- Document every public function, type, macro and struct field with a
  `/** @brief ... */` comment, or `/**< ... */` after a field or enum value.
- Put each declaration in the `@defgroup` of its section, between `@{` and
  `@}`. Each group becomes one page.

Edit the header, not `docs/`, then regenerate:

```sh
make docs                                                  # Doxygen on PATH
make docs DOXYGEN="/c/Program Files/doxygen/bin/doxygen"  # or point at it
```

Doxygen fails the build if anything public is undocumented. Commit the header
and the regenerated docs together; CI regenerates them with the same versions
(Doxygen 1.18.0, moxygen 2.1.19) and fails if they differ.

## Unit tests

```sh
make -C tests                          # build and run with cc
make -C tests CC=clang                 # any C99 compiler
make -C tests CC=gcc SANITIZE=         # without ASan/UBSan, e.g. MinGW gcc on Windows
```

The unit tests don't need CEdev or a calculator. They compile the library with
the host compiler against the headers in `tests/stubs/`. Drawing goes to an
in-memory 320×240 framebuffer and a stand-in keypad presses a scripted list of
keys, so layout, clipping, focus, widgets, key handling and the run loop are
tested through the public API.

## Hardware tests

`tests/hw/` holds small CE programs that run in
[CEmu](https://github.com/CE-Programming/CEmu)'s `cemu-autotester`. For each
test, the autotester launches the program, presses keys, and compares CRCs of
video memory with the recorded screens. These tests cover what the unit tests
can't: the real eZ80 compiler (24-bit `int`), graphx, the keypad and the clock.

### What you need

- `cemu-autotester`, found on `PATH` (CEdev ships it) or set in
  `CEMU_AUTOTESTER`.
- `AUTOTESTER_ROM`: a TI-84 Plus CE ROM image with the
  [CE C libraries](https://github.com/CE-Programming/libraries/releases)
  (`clibs.8xg`) installed. It must be one of:
  - **OS 5.4 or older.** The autotester starts programs with `Asm(prgmNAME)`.
  - **OS 5.5 or newer, jailbroken with [arTIfiCE](https://yvantt.github.io/arTIfiCE/)**,
    with its `AsmHook2` app installed. Each test first runs AsmHook2 from the
    Apps menu, then runs the program from the PRGM menu.

The runner picks the launch method from the ROM: arTIfiCE if AsmHook2 is on it,
`Asm(` otherwise. Set `HW_LAUNCH=asm` or `HW_LAUNCH=artifice` (or pass
`--launch`) to choose. On an arTIfiCE ROM, the program is picked by the first
letter of its name, so the ROM mustn't hold another program that starts with the
same letter and sorts before it.

### Running

```sh
export AUTOTESTER_ROM=/path/to/ti84ce.rom
make hw-test                           # build and run all of them
make hw-test HW_ARGS="layout widgets"  # or some of them
make hw-record                         # re-record the CRCs of failing screens
python tests/hw/run.py --help          # all options
```

| Test | Checks |
|---|---|
| `canary` | Only the setup: a graphx program launches and exits. If it fails, check the ROM first |
| `glyphs` | Every character code, reverse video, box-drawing joins, word wrap |
| `layout` | Fixed, percent and weighted-fill sizes, nesting, clipping, hide/show reflow, destroying a subtree |
| `controls` | Checkboxes, a custom widget built with a key handler, and buttons: toggling, custom change events, focus markers, submitting |
| `widgets` | List, input and a text log driven by key presses: wrap-around, `[enter]`, focus moved by the app on `[vars]`, alpha and alpha lock, `[del]`, scrollback |
| `ticks` | `term_set_tick` with the real `clock()`: 20 ticks of 100 ms arrive on time |
| `scenes` | Switching between two scenes: each keeps its content and list selection, a scene's handler refocuses on entry, and a hidden scene can be printed into |
| `overlays` | A centered dialog over a list: focus moved into it, typing, submitting and cancelling, focus given back, and the screen underneath restored exactly |
| `perf` | Update times when nothing changes, when one row changes and when the whole screen changes stay within budget (the one-row budget is the 50 ms goal); a miss shows the measured time |
| `selfcheck` | Checks that run on the calculator and read pixels back from the screen: layout, 24-bit `printf`, clipping, the log ring, focus, panel limits |

Every test ends by pressing `[clear]` and checking for a cleared home screen.

When a screen doesn't match, the runner saves it as a PNG in
`tests/hw/build/<test>/`, next to the autotester's log.

### Recording screens

After an intended change to what's on screen, run `make hw-record`. It writes
the new CRCs into each test's `autotest.json`. **Open the PNGs and check them
before committing**, because a recording accepts whatever was on screen.

- Screens drawn by titrmlib (hashed over `vram_8_size`) have exactly one
  correct CRC, so recording replaces it.
- Home-screen CRCs (hashed over `vram_16_size`) differ between OS versions,
  so recording adds the new one to the list.

### Adding a hardware test

1. Create `tests/hw/<name>/main.c`. Keep the final screen the same on every
   run: no clock values or counters, unless they only show on failure.
2. Pick a program name of at most 8 characters starting with a letter, and add
   `HW_NAME_<name> = <PROGRAM>` to `project.mk`.
3. Create `tests/hw/<name>/autotest.json` with that program as `target`. Start
   the sequence with `action|launch`, use `hashWait` for each screen to check,
   and end with `key|clear` and the home-screen hash (copy it from another
   test). Use `"00000000"` as the CRC of new screens.
4. Run `make hw-record HW_ARGS=<name>`, check the PNGs, then run
   `make hw-test HW_ARGS=<name>` to confirm that it passes.

## CI

`.github/workflows/ci.yml` runs on every branch push and pull request, and
before every release:

- **test**: the unit tests under gcc and clang with ASan and UBSan
- **font**: reruns `tools/gen_font.py` and `make docs` and fails if `src/` or `docs/` change
- **build**: builds every example and hardware test program with CEdev

The hardware tests themselves don't run in CI because they need a ROM. Run
them locally when you change rendering, input or timing.

## Releasing

Set `TITRM_VERSION` in `src/titrm.h` to the new version (without the `v`),
run `make docs`, and commit. Then push a version tag to that commit:

```sh
git tag v1.0.0
git push origin v1.0.0
```

`.github/workflows/release.yml` fails if `TITRM_VERSION` doesn't match the tag.
Otherwise it runs CI, then creates a **draft** release named
after the tag. It attaches `titrmlib-<tag>.zip`, which holds `src/*.c`,
`src/*.h` and the licenses, and a short "how to use" description. Add the
changes to the description on GitHub, then publish the draft.

## Reporting issues

**Bugs**: include

- what you did, what you expected, and what happened instead
- the smallest program that shows it, if you can reduce it
- the titrmlib commit, the CEdev version (`cedev-config --version`) and the
  calculator OS version
- whether it happened on a calculator or in CEmu
- a screenshot, or the PNG from `tests/hw/build/` if a hardware test failed

**Feature requests**: describe the screen or interaction you're trying to
build, and what's missing from the current API. Check [ROADMAP.md](ROADMAP.md)
first.

## Pull requests

- Start commit subjects with a bracketed tag, as in the existing history:
  `[dev] ...`, `[test] ...`.
- Run the unit tests. If you touch rendering, input or timing, run the
  hardware tests too.
- If screens change on purpose, re-record them and check the PNGs.
- Update `README.md` and regenerate `docs/` for public API changes, and `ROADMAP.md` when you finish
  or add a planned item.
