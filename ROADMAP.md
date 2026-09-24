# Roadmap

## Planned

- Check the styles (v0.4.0) on a real TI-84 Plus CE, and pick the
  strikethrough row there (`STRIKE_ROW` in `titrm.c`, row 3 for now).
- Check the CP437 glyphs (v0.3.0) on a real TI-84 Plus CE; the roughest at
  5×7 are ½ ¼ ₧ ♫ and ▓.

## Known issues

- **Hardware tests don't run in CI.** They need a TI-84 Plus CE ROM, which can't
  be distributed. CI only builds the test programs.

## Out of scope

- Several titrmlib instances sharing the screen with other UI. One instance owns
  the whole screen for one program.
- FontLib, or choosing a font at runtime.
