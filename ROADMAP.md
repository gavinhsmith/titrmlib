# Roadmap

## Planned

- **Color.** A palette and per-cell foreground/background colors. Today every
  cell is light-on-dark or reversed. The cell drawing routine (`draw_cell` in
  `src/titrm.c`) copies pixels from a table built for those two colors, so it
  needs per-color tables (or another approach) when this lands.
- **Real hardware check.** The 5×7 font and the small icons in 0x80–0xFF have
  only been checked in CEmu. They need a look on a physical TI-84 Plus CE.
- **Tincan / TINCLIB screens.** Device pickers and connection-status screens
  for the sibling networking libraries, built from the existing widgets.

## Known issues

- **Quick repeated key presses can merge.** `os_GetCSC()` holds a single key
  between polls, so two presses within one frame count as one. A typical update
  takes about 105 ms and a full-screen redraw about 400 ms (emulated).
- **Hardware tests don't run in CI.** They need a TI-84 Plus CE ROM, which can't
  be distributed. CI only builds the test programs.

## Out of scope

- Several titrmlib instances sharing the screen with other UI. One instance owns
  the whole screen for one program.
- FontLib, or choosing a font at runtime.
