# Roadmap

## Planned

- **Phase 2:** retained panels and change-based rendering, a key queue,
  scenes, overlays, a revised widget set and color. The agreed design is in
  [DESIGN.md](DESIGN.md).

## Known issues

- **Quick repeated key presses can merge.** `os_GetCSC()` holds a single key
  between polls, so two presses within one frame count as one. A typical update
  takes about 105 ms and a full-screen redraw about 400 ms (emulated). Phase 2
  addresses both.
- **Hardware tests don't run in CI.** They need a TI-84 Plus CE ROM, which can't
  be distributed. CI only builds the test programs.

## Out of scope

- Several titrmlib instances sharing the screen with other UI. One instance owns
  the whole screen for one program.
- FontLib, or choosing a font at runtime.
