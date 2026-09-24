# Roadmap

## Planned

- Re-record `demo.gif` with the phase 2 demo.
- Check the CP437 glyphs (v0.3.0) on a real TI-84 Plus CE; the roughest at
  5×7 are ½ ¼ ₧ ♫ and ▓.

## Known issues

- **Hardware tests don't run in CI.** They need a TI-84 Plus CE ROM, which can't
  be distributed. CI only builds the test programs.

## Out of scope

- Several titrmlib instances sharing the screen with other UI. One instance owns
  the whole screen for one program.
- FontLib, or choosing a font at runtime.
