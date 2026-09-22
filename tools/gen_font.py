#!/usr/bin/env python3
"""Convert petabyt/font's font.h into titrmlib's packed 5x7 glyph table.

Reads   tools/petabyt-font/font.h                        (MIT, see LICENSE there)
Writes  src/titrm_font.c    const term_glyph_t term_font[256]
        src/titrm_chars.h   TERM_CH_* names for the 0x80-0xFF glyphs
        src/FONT.md         code -> glyph -> purpose mapping table

Run from anywhere:  python tools/gen_font.py

Everything the source font gets wrong or omits is fixed up *here* (see the
FIXES section) so the generated files can be regenerated at any time and the
decisions stay documented in one place.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC_FONT = ROOT / "tools" / "petabyt-font" / "font.h"
OUT_C = ROOT / "src" / "titrm_font.c"
OUT_H = ROOT / "src" / "titrm_chars.h"
OUT_MD = ROOT / "src" / "FONT.md"

W, H = 5, 7

# --------------------------------------------------------------------------
# Parse the source font
# --------------------------------------------------------------------------

ENTRY_RE = re.compile(
    r"\{\s*('(?:\\.|[^'\\])'|\d+)\s*,\s*\{\s*(?:/\*.*?\*/\s*)?((?:\"[^\"]*\"\s*,?\s*){7})\}\}",
    re.S,
)


def parse_key(tok):
    if tok.startswith("'"):
        body = tok[1:-1]
        return ord(body[1]) if body.startswith("\\") else ord(body)
    return int(tok)  # raw integer key, e.g. the smiley keyed as `1`


def parse_source(text):
    """Return {key: [7 strings]}. Later duplicates win (see FIXES)."""
    glyphs, dupes = {}, []
    for m in ENTRY_RE.finditer(text):
        key = parse_key(m.group(1))
        rows = re.findall(r'"([^"]*)"', m.group(2))
        assert len(rows) == H and all(len(r) == W for r in rows), (key, rows)
        if key in glyphs:
            dupes.append(key)
        glyphs[key] = rows
    return glyphs, dupes


# --------------------------------------------------------------------------
# FIXES: gaps and oddities in the source font
# --------------------------------------------------------------------------
#
#  * The source keys its entries by char, but ends with a {0, all-filled}
#    "null terminator". We drop that entry and reuse the idea as TERM_CH_BLOCK.
#  * '-' and '_' each appear twice. '_' entries are identical. The two '-'
#    entries differ (3px dash on row 3 vs 5px dash on row 4); the later one is
#    kept because it lines up with '+' and '=' and spans the full glyph width.
#  * Entry keyed by raw integer 1 is a smiley face. Not a control character
#    we need; it is moved to 0x9D as TERM_CH_SMILE.
#  * Missing entirely from the source: $ & @ [ ] \ ^ |   (CLAUDE.md only
#    listed '|'). Drawn by hand below; '\' is the mirror of the source's '/'.

EXTRA_ASCII = {
    "|": ["  #  "] * 7,
    "[": ["###  ", "#    ", "#    ", "#    ", "#    ", "#    ", "###  "],
    "]": ["  ###", "    #", "    #", "    #", "    #", "    #", "  ###"],
    "^": ["  #  ", " # # ", "#   #", "     ", "     ", "     ", "     "],
    "$": ["  #  ", " ####", "# #  ", " ### ", "  # #", "#### ", "  #  "],
    "&": [" ##  ", "#  # ", " ##  ", " ## #", "#  # ", "#  # ", " ## #"],
    "@": [" ### ", "#   #", "# ###", "# # #", "# ###", "#    ", " ####"],
}

#  * The source draws narrow glyphs flush-left in the 5px box (e.g. 'i' is a
#    single column 0). In a monospace grid that leaves a 4-5px hole before the
#    next character. The thin letters/digit are redrawn with serifs so they fill
#    the cell like the rest of the alphabet; every other glyph whose blank
#    margins are lopsided by 2+ columns is shifted to sit centered.

REDRAWN_ASCII = {
    "I": [" ### ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "],
    "i": ["  #  ", "     ", " ##  ", "  #  ", "  #  ", "  #  ", " ### "],
    "l": [" ##  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "],
    "1": ["  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "],
}


def center(rows):
    """Shift a glyph horizontally so its left/right blank margins differ by <= 1."""
    cols = [c for c in range(W) if any(r[c] == "#" for r in rows)]
    if not cols:
        return rows
    left, right = min(cols), W - 1 - max(cols)
    shift = (right - left) // 2  # +ve = move right
    if shift == 0:
        return rows
    if shift > 0:
        return [" " * shift + r[:W - shift] for r in rows]
    return [r[-shift:] + " " * -shift for r in rows]

# --------------------------------------------------------------------------
# Reserved range 0x80-0xFF: box drawing + icons
# --------------------------------------------------------------------------
#
# Box glyphs are "connected": at draw time the renderer stretches them across
# the inter-character / inter-line gap (repeating the right column and bottom
# row) so lines meet across cells. Their lines run on column 2 and row 3.
#
# (code, NAME, purpose, art)

V = "  #  "
BOX = [
    (0x80, "HLINE",  "box: horizontal",        ["     ", "     ", "     ", "#####", "     ", "     ", "     "]),
    (0x81, "VLINE",  "box: vertical",          [V] * 7),
    (0x82, "TL",     "box: top-left corner",   ["     ", "     ", "     ", "  ###", V, V, V]),
    (0x83, "TR",     "box: top-right corner",  ["     ", "     ", "     ", "###  ", V, V, V]),
    (0x84, "BL",     "box: bottom-left corner", [V, V, V, "  ###", "     ", "     ", "     "]),
    (0x85, "BR",     "box: bottom-right corner", [V, V, V, "###  ", "     ", "     ", "     "]),
    (0x86, "LTEE",   "box: tee, opens right",  [V, V, V, "  ###", V, V, V]),
    (0x87, "RTEE",   "box: tee, opens left",   [V, V, V, "###  ", V, V, V]),
    (0x88, "TTEE",   "box: tee, opens down",   ["     ", "     ", "     ", "#####", V, V, V]),
    (0x89, "BTEE",   "box: tee, opens up",     [V, V, V, "#####", "     ", "     ", "     "]),
    (0x8A, "CROSS",  "box: cross",             [V, V, V, "#####", V, V, V]),
]
CONNECTED = {c for c, *_ in BOX}

ICONS = [
    (0x90, "CHECK",   "status: checkmark",
     ["     ", "    #", "    #", "#  # ", " # # ", "  #  ", "     "]),
    (0x91, "CROSSMARK", "status: x mark",
     ["     ", "#   #", " # # ", "  #  ", " # # ", "#   #", "     "]),
    (0x92, "DOT",     "status: filled dot",
     ["     ", " ### ", "#####", "#####", "#####", " ### ", "     "]),
    (0x93, "DOT_EMPTY", "status: empty dot",
     ["     ", " ### ", "#   #", "#   #", "#   #", " ### ", "     "]),
    (0x94, "ARROW_R", "selection / collapsed",
     ["     ", "#    ", "##   ", "###  ", "##   ", "#    ", "     "]),
    (0x95, "ARROW_D", "expanded / scroll down",
     ["     ", "     ", "#####", " ### ", "  #  ", "     ", "     "]),
    (0x96, "ARROW_U", "scroll up",
     ["     ", "     ", "  #  ", " ### ", "#####", "     ", "     "]),
    (0x98, "SIG0",    "signal strength 0",
     ["     ", "     ", "     ", "     ", "     ", "     ", "# # #"]),
    (0x99, "SIG1",    "signal strength 1",
     ["     ", "     ", "     ", "     ", "#    ", "#    ", "# # #"]),
    (0x9A, "SIG2",    "signal strength 2",
     ["     ", "     ", "  #  ", "  #  ", "# #  ", "# #  ", "# # #"]),
    (0x9B, "SIG3",    "signal strength 3 (full)",
     ["    #", "    #", "  # #", "  # #", "# # #", "# # #", "# # #"]),
    (0x9C, "SHADE",   "progress track / scrollbar",
     ["# # #", " # # ", "# # #", " # # ", "# # #", " # # ", "# # #"]),
    (0x9E, "BLOCK",   "solid block: progress fill / scrollbar thumb",
     ["#####"] * 7),
    (0x9D, "SMILE",   "smiley (from source font's raw code 1)", None),  # filled in
]
CONNECTED.add(0x9E)


def to_bits(row):
    v = 0
    for ch in row:
        v = (v << 1) | (1 if ch == "#" else 0)
    return v


def main():
    glyphs, dupes = parse_source(SRC_FONT.read_text())
    smiley = glyphs.pop(1)
    glyphs.pop(0, None)  # null terminator

    table = {}
    purpose = {}

    for key, rows in glyphs.items():
        if not 0x20 <= key <= 0x7E:
            sys.exit(f"unexpected source key {key!r}")
        if chr(key) in REDRAWN_ASCII:
            table[key] = REDRAWN_ASCII[chr(key)]
            purpose[key] = "ascii (redrawn: source glyph too narrow)"
        else:
            table[key] = center(rows)
            purpose[key] = "ascii (petabyt/font)"
    for ch, rows in EXTRA_ASCII.items():
        assert ord(ch) not in table, ch
        table[ord(ch)] = rows
        purpose[ord(ch)] = "ascii (hand-drawn: missing from source)"
    # '\' = mirror of '/'
    table[ord("\\")] = [r[::-1] for r in table[ord("/")]]
    purpose[ord("\\")] = "ascii (mirror of '/': missing from source)"

    names = {}
    for code, name, why, art in BOX + ICONS:
        if art is None:
            art = smiley
        assert len(art) == H and all(len(r) == W for r in art), name
        table[code] = art
        purpose[code] = why
        names[code] = name

    missing = [c for c in range(0x20, 0x7F) if c not in table]
    assert not missing, f"unfilled ASCII: {missing}"

    # ---- titrm_font.c --------------------------------------------------
    lines = [
        "/* GENERATED by tools/gen_font.py from petabyt/font (MIT). Do not edit. */",
        "#include \"titrm_font.h\"",
        "",
        "/* one byte per row, bits 4..0 = pixels left to right */",
        "const term_glyph_t term_font[256] = {",
    ]
    for code in range(256):
        if code in table:
            rows = ", ".join(f"0x{to_bits(r):02X}" for r in table[code])
            if code in names:
                label = names[code]
            elif code == ord("\\"):
                label = "backslash"  # a literal '\' at end of a comment would splice lines
            else:
                label = chr(code) if code > 0x20 else "space"
            lines.append(f"    [0x{code:02X}] = {{{{ {rows} }}}}, /* {label} */")
    lines += ["};", ""]
    OUT_C.write_text("\n".join(lines), newline="\n")

    # ---- titrm_chars.h -------------------------------------------------
    h = [
        "/* GENERATED by tools/gen_font.py. Do not edit. */",
        "#ifndef TITRM_CHARS_H",
        "#define TITRM_CHARS_H",
        "",
        "/* Glyphs in the reserved 0x80-0xFF range. See FONT.md for the full table. */",
    ]
    for code in sorted(names):
        h.append(f"#define TERM_CH_{names[code]:<10} 0x{code:02X}")
    h += [
        "",
        "/* The same glyphs as string literals, for building text:",
        " *   TERM_S_CHECK \"Done\"  ->  \"\\x90\" \"Done\"",
        " * (adjacent literals concatenate after escapes are processed, so a",
        " * following hex digit can't be swallowed into the escape.) */",
    ]
    for code in sorted(names):
        h.append(f"#define TERM_S_{names[code]:<10} \"\\x{code:02X}\"")
    h += ["", "#endif", ""]
    OUT_H.write_text("\n".join(h), newline="\n")

    # ---- FONT.md -------------------------------------------------------
    md = [
        "# titrmlib font map",
        "",
        "Generated by `tools/gen_font.py` from [petabyt/font](https://github.com/petabyt/font)"
        " (MIT, `tools/petabyt-font/LICENSE`). 5x7 glyphs, packed one byte per row.",
        "",
        "Fixes applied to the source font (see the script for detail):",
        "",
        "- `-` and `_` were each defined twice; kept the later `-` (5px, row 4).",
        "- Raw code `1` was a smiley face; now `TERM_CH_SMILE` (0x9D).",
        "- Missing `$ & @ [ ] \\ ^ |` drawn by hand (`\\` mirrors `/`).",
        "- `I i l 1` redrawn with serifs to fill the cell; other narrow glyphs"
        " (`! . ' : ( )` etc.) centered instead of flush-left, so they don't leave a gap.",
        "- Source's all-filled null terminator dropped (`TERM_CH_BLOCK` is the same idea).",
        "",
        "## 0x20-0x7E: ASCII",
        "",
        "All 95 printable characters; standard ASCII positions.",
        "",
        "## 0x80-0xFF: reserved",
        "",
        "| Code | Name | Purpose | Glyph |",
        "|------|------|---------|-------|",
    ]
    for code in sorted(names):
        md.append(f"| 0x{code:02X} | `TERM_CH_{names[code]}` | {purpose[code]} | "
                  f"<pre>{chr(10).join(table[code]).replace(' ', '.')}</pre> |")
    md += [
        "",
        "Box-drawing glyphs and `TERM_CH_BLOCK` are *connected*: the renderer extends them across the",
        "1px column/row gap between cells so lines join. The `.` in the art above is an empty pixel.",
        "",
        "Unassigned codes in 0x80-0xFF render blank.",
        "",
    ]
    OUT_MD.write_text("\n".join(md), newline="\n")

    print(f"parsed {len(glyphs)} source glyphs; duplicates resolved: "
          f"{sorted(chr(d) for d in set(dupes))}")
    print(f"wrote {OUT_C.name}, {OUT_H.name}, {OUT_MD.name}")


if __name__ == "__main__":
    main()
