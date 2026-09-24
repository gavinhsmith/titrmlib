# Roadmap

## Planned

### v0.4: formatting

Text styles and tabs.

**Styles.** New attribute bits next to `TERM_ATTR_REVERSE` (1):
`TERM_ATTR_BOLD` (2), `TERM_ATTR_ITALIC` (4), `TERM_ATTR_UNDERLINE` (8) and
`TERM_ATTR_STRIKE` (16). They combine freely and work anywhere an attribute
does: `term_panel_set_attr`, widgets, and `term_panel_set_focus_attr`.

- `draw_cell` changes each glyph row's 6-bit mask before the `row_pixels`
  lookup:
  - bold: `row | row >> 1`, into the gap column
  - italic: rows 0–2 shifted one pixel right
  - underline: row 7 filled
  - strikethrough: one middle row filled (row 3 or 4, picked on real hardware)
- Underline and strikethrough fill the gap column too, so they join across
  cells. Bold and italic together make wide letters that may touch; that's
  accepted.
- Box drawing and blocks (0xB3–0xDF) ignore styles.
- `term_cell_t` gets a fourth byte for the style bits (REVERSE is still
  resolved into the colors). That's ~3.2 KB more for `grid` and `shown`, plus
  panel cells. A 4-byte cell may index faster than a 3-byte one; check `lto.s`.
- Unstyled cells keep the current fast path in `draw_cell`.

**Inline styles.** ESC (0x1B) followed by `0x40 | bits` sets the style inside
a string, taking no width. Named string literals `TERM_S_NORMAL`,
`TERM_S_BOLD`, `TERM_S_ITALIC`, `TERM_S_UNDERLINE`, `TERM_S_STRIKE` and
`TERM_S_REVERSE` (e.g. `"\x1b" "B"`: a hex escape needs the split).
- `term_panel_putc`: the escape sets the panel's attribute, like
  `term_panel_set_attr`.
- Text widgets and logs: `text_layout` keeps a style per character and skips
  escapes when measuring words. A style lasts until the next escape or the end
  of the line, so trimming a log's oldest lines can't leave a style behind.
  Styles are added to the widget's own attribute, so focus still shows.
- ESC's CP437 glyph (←) can no longer be printed; it has no `TERM_CH_*` name.

**Tabs.** Stops every 4 columns; 0x09 (○) can no longer be printed.
- `term_panel_putc`: moves the cursor to the next stop without painting,
  clamped at 255.
- `text_layout`: expands to spaces, following the existing space rules
  (indentation kept, dropped on wrapped rows); the word scanner stops at tabs.
- `put_str` (list items, button labels): expands to spaces.

**Checks.**
- Unit tests for each style's row masks, inline escapes (including across a
  log trim), tabs in panels, text widgets and lists, and cell size.
- A `styles` hardware test with recorded screens; re-record the screens that
  change.
- `perf` within its budgets.
- Styles checked on a real TI-84 Plus CE.
- `TITRM_VERSION` to `0.4.0`, docs regenerated, README and AGENTS.md updated.

**Not in v0.4:** dim/faint, blink, right alignment, a settable tab width,
more printf specifiers.

### Other

- Check the CP437 glyphs (v0.3.0) on a real TI-84 Plus CE; the roughest at
  5×7 are ½ ¼ ₧ ♫ and ▓.

## Known issues

- **Hardware tests don't run in CI.** They need a TI-84 Plus CE ROM, which can't
  be distributed. CI only builds the test programs.

## Out of scope

- Several titrmlib instances sharing the screen with other UI. One instance owns
  the whole screen for one program.
- FontLib, or choosing a font at runtime.
