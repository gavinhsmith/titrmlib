#!/usr/bin/env python3
"""Build titrmlib's packed 5x7 glyph table: ASCII from petabyt/font's font.h,
everything else from code page 437, drawn here.

Reads   tools/petabyt-font/font.h                        (MIT, see LICENSE there)
Writes  src/titrm_font.c    const term_glyph_t term_font[256]
        src/titrm_chars.h   TERM_CH_* names for the glyphs titrmlib uses
        src/FONT.md         code -> CP437 character -> glyph table

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
#  * Entry keyed by raw integer 1 is a smiley face. It is CP437's 0x01 (☺),
#    TERM_CH_SMILE.
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
# 0x01-0x1F and 0x7F-0xFF: code page 437
# --------------------------------------------------------------------------
#
# The rest of the table follows the IBM PC character set (CP437), so a code
# prints the character it does there. Exceptions:
#
#  * 0x13-0x16 (CP437's double-bang, pilcrow, section sign and thick bar) hold
#    the signal-strength icons, kept contiguous so TERM_CH_SIG0 + level works.
#  * 0x00 and 0xFF (NBSP) are blank. 0x00 ends a string and 0x0A/0x0D are
#    newline/return to term_panel_print, so those three need term_put.
#
# Box-drawing glyphs 0xB3-0xDA are generated from their arms (box() below).
# 0xB3-0xDF (box drawing and the solid/half blocks) are "connected": the
# renderer stretches them across the gap between cells so they join up.

CONNECTED_FIRST, CONNECTED_LAST = 0xB3, 0xDF

# TERM_CH_* names, for the glyphs titrmlib and its apps use.
NAMES = {
    0x01: "SMILE", 0x10: "ARROW_R", 0x1E: "ARROW_U", 0x1F: "ARROW_D",
    0x13: "SIG0", 0x14: "SIG1", 0x15: "SIG2", 0x16: "SIG3",
    0xB1: "SHADE", 0xDB: "BLOCK",
    0xC4: "HLINE", 0xB3: "VLINE", 0xDA: "TL", 0xBF: "TR", 0xC0: "BL",
    0xD9: "BR", 0xC3: "LTEE", 0xB4: "RTEE", 0xC2: "TTEE", 0xC1: "BTEE",
    0xC5: "CROSS",
}

# Unicode for each code, for FONT.md and the comments in titrm_font.c.
CP437 = (
    "\0☺☻♥♦♣♠•◘○◙♂♀♪♫☼►◄↕‼¶§▬↨↑↓→←∟↔▲▼"
    + "".join(chr(c) for c in range(0x20, 0x7F)) + "⌂"
    + "ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒáíóúñÑªº¿⌐¬½¼¡«»"
    + "░▒▓│┤╡╢╖╕╣║╗╝╜╛┐└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀"
    + "αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■\u00a0"
)
assert len(CP437) == 256


def art(s):
    """'rows|separated|by|bars' -> 7 rows of '#' and ' ' ('.' is off)."""
    rows = s.split("|")
    assert len(rows) == H and all(len(r) == W for r in rows), s
    return [r.replace(".", " ") for r in rows]


HAND = {
    0x02: ".....|.###.|#.#.#|#####|#...#|.###.|.....",  # ☻
    0x03: ".....|.#.#.|#####|#####|.###.|..#..|.....",  # ♥
    0x04: ".....|..#..|.###.|#####|.###.|..#..|.....",  # ♦
    0x05: ".###.|.###.|#.#.#|#####|#.#.#|..#..|.###.",  # ♣
    0x06: "..#..|.###.|#####|#####|#.#.#|..#..|.###.",  # ♠
    0x07: ".....|.....|..#..|.###.|..#..|.....|.....",  # •
    0x08: "#####|#####|##.##|#...#|##.##|#####|#####",  # ◘
    0x09: ".....|.###.|#...#|#...#|#...#|.###.|.....",  # ○
    0x0A: "#####|#...#|.###.|.###.|.###.|#...#|#####",  # ◙
    0x0B: "..###|...##|.##.#|#..#.|#..#.|.##..|.....",  # ♂
    0x0C: ".###.|#...#|#...#|.###.|..#..|.###.|..#..",  # ♀
    0x0D: "..#..|..##.|..#.#|..#..|.##..|###..|.#...",  # ♪
    0x0E: ".####|.#..#|.#..#|.#..#|.#.##|##.##|##...",  # ♫
    0x0F: "..#..|#.#.#|.###.|##.##|.###.|#.#.#|..#..",  # ☼
    0x10: ".....|#....|##...|###..|##...|#....|.....",  # ►
    0x11: ".....|....#|...##|..###|...##|....#|.....",  # ◄
    0x12: "..#..|.###.|#.#.#|..#..|#.#.#|.###.|..#..",  # ↕
    0x13: ".....|.....|.....|.....|.....|.....|#.#.#",  # signal 0
    0x14: ".....|.....|.....|.....|#....|#....|#.#.#",  # signal 1
    0x15: ".....|.....|..#..|..#..|#.#..|#.#..|#.#.#",  # signal 2
    0x16: "....#|....#|..#.#|..#.#|#.#.#|#.#.#|#.#.#",  # signal 3
    0x17: "..#..|.###.|..#..|..#..|.###.|..#..|#####",  # ↨
    0x18: "..#..|.###.|#.#.#|..#..|..#..|..#..|..#..",  # ↑
    0x19: "..#..|..#..|..#..|..#..|#.#.#|.###.|..#..",  # ↓
    0x1A: ".....|..#..|...#.|#####|...#.|..#..|.....",  # →
    0x1B: ".....|..#..|.#...|#####|.#...|..#..|.....",  # ←
    0x1C: ".....|.....|#....|#....|#####|.....|.....",  # ∟
    0x1D: ".....|.....|.#.#.|#####|.#.#.|.....|.....",  # ↔
    0x1E: ".....|.....|..#..|.###.|#####|.....|.....",  # ▲
    0x1F: ".....|.....|#####|.###.|..#..|.....|.....",  # ▼
    0x7F: "..#..|.#.#.|#...#|#...#|#...#|#####|.....",  # ⌂
    0x91: ".....|.....|##.#.|..#.#|.####|#.#..|.####",  # æ
    0x92: ".####|#.#..|#.#..|#####|#.#..|#.#..|#.###",  # Æ
    0x9B: "..#..|.####|#.#..|#.#..|#.#..|.####|..#..",  # ¢
    0x9C: "..##.|.#..#|.#...|###..|.#...|.#...|#####",  # £
    0x9D: "#...#|.#.#.|#####|..#..|#####|..#..|..#..",  # ¥
    0x9E: "###..|#..#.|###..|#..#.|#.###|#..#.|#..##",  # ₧
    0x9F: "...##|..#..|..#..|.###.|..#..|..#..|##...",  # ƒ
    0xA6: ".###.|....#|.####|#...#|.####|.....|#####",  # ª
    0xA7: ".###.|#...#|#...#|.###.|.....|#####|.....",  # º
    0xA9: ".....|.....|#####|#....|#....|.....|.....",  # ⌐
    0xAA: ".....|.....|#####|....#|....#|.....|.....",  # ¬
    0xAB: "#....|#..#.|#.#..|.#.##|#...#|...#.|...##",  # ½
    0xAC: "#....|#..#.|#.#..|.#.#.|#..##|....#|....#",  # ¼
    0xAE: ".....|..#.#|.#.#.|#.#..|.#.#.|..#.#|.....",  # «
    0xAF: ".....|#.#..|.#.#.|..#.#|.#.#.|#.#..|.....",  # »
    0xB0: "#...#|..#..|#...#|..#..|#...#|..#..|#...#",  # ░
    0xB1: "#.#.#|.#.#.|#.#.#|.#.#.|#.#.#|.#.#.|#.#.#",  # ▒
    0xB2: ".###.|##.##|.###.|##.##|.###.|##.##|.###.",  # ▓
    0xDB: "#####|#####|#####|#####|#####|#####|#####",  # █
    0xDC: ".....|.....|.....|.....|#####|#####|#####",  # ▄
    0xDD: "###..|###..|###..|###..|###..|###..|###..",  # ▌
    0xDE: "...##|...##|...##|...##|...##|...##|...##",  # ▐
    0xDF: "#####|#####|#####|#####|.....|.....|.....",  # ▀
    0xE0: ".....|.....|.##.#|#..#.|#..#.|#..#.|.##.#",  # α
    0xE1: ".###.|#...#|#..#.|#.#..|#..#.|#...#|#.##.",  # ß
    0xE2: "#####|#....|#....|#....|#....|#....|#....",  # Γ
    0xE3: ".....|.....|#####|.#.#.|.#.#.|.#.#.|.#..#",  # π
    0xE4: "#####|#....|.#...|..#..|.#...|#....|#####",  # Σ
    0xE5: ".....|.....|.####|#..#.|#..#.|#..#.|.##..",  # σ
    0xE6: ".....|.....|#..#.|#..#.|#..#.|###.#|#....",  # µ
    0xE7: ".....|.....|.####|#.#..|..#..|..#..|..##.",  # τ
    0xE8: "..#..|.###.|#.#.#|#.#.#|#.#.#|.###.|..#..",  # Φ
    0xE9: ".###.|#...#|#...#|#####|#...#|#...#|.###.",  # Θ
    0xEA: ".###.|#...#|#...#|#...#|.#.#.|.#.#.|##.##",  # Ω
    0xEB: "..##.|.#...|..#..|.###.|#...#|#...#|.###.",  # δ
    0xEC: ".....|.....|.#.#.|#.#.#|#.#.#|.#.#.|.....",  # ∞
    0xED: "....#|...#.|.###.|#.#.#|#.#.#|.###.|.#...",  # φ
    0xEE: ".....|.....|.####|#....|.###.|#....|.####",  # ε
    0xEF: ".....|.###.|#...#|#...#|#...#|#...#|.....",  # ∩
    0xF0: ".....|#####|.....|#####|.....|#####|.....",  # ≡
    0xF1: "..#..|..#..|#####|..#..|..#..|.....|#####",  # ±
    0xF2: ".#...|..#..|...#.|..#..|.#...|.....|.###.",  # ≥
    0xF3: "...#.|..#..|.#...|..#..|...#.|.....|.###.",  # ≤
    0xF4: "..##.|..#.#|..#..|..#..|..#..|..#..|..#..",  # ⌠ (⌡ is it rotated)
    0xF6: ".....|..#..|.....|#####|.....|..#..|.....",  # ÷
    0xF7: ".....|.##.#|#.##.|.....|.##.#|#.##.|.....",  # ≈
    0xF8: ".##..|#..#.|#..#.|.##..|.....|.....|.....",  # °
    0xF9: ".....|.....|.....|.##..|.##..|.....|.....",  # ∙
    0xFA: ".....|.....|.....|..#..|.....|.....|.....",  # ·
    0xFB: "...##|...#.|...#.|#..#.|.#.#.|..#..|.....",  # √
    0xFC: "###..|#.#..|#.#..|.....|.....|.....|.....",  # ⁿ
    0xFD: "##...|..#..|.#...|###..|.....|.....|.....",  # ²
    0xFE: ".....|.###.|.###.|.###.|.###.|.....|.....",  # ■
}

# Accents take rows 0-1, above a lowercase letter (rows 2-6) or a capital
# squashed to 5 rows.
ACCENTS = {
    "grave": ".#...|..#..", "acute": "...#.|..#..", "circ": "..#..|.#.#.",
    "dia": ".#.#.|.....", "ring": ".###.|.#.#.", "tilde": ".##.#|#.##.",
}
ACCENTED = {  # code: (base letter, accent); "ı" is a dotless i
    0x81: ("u", "dia"), 0x82: ("e", "acute"), 0x83: ("a", "circ"), 0x84: ("a", "dia"),
    0x85: ("a", "grave"), 0x86: ("a", "ring"), 0x88: ("e", "circ"), 0x89: ("e", "dia"),
    0x8A: ("e", "grave"), 0x8B: ("ı", "dia"), 0x8C: ("ı", "circ"), 0x8D: ("ı", "grave"),
    0x8E: ("A", "dia"), 0x8F: ("A", "ring"), 0x90: ("E", "acute"), 0x93: ("o", "circ"),
    0x94: ("o", "dia"), 0x95: ("o", "grave"), 0x96: ("u", "circ"), 0x97: ("u", "grave"),
    0x98: ("y", "dia"), 0x99: ("O", "dia"), 0x9A: ("U", "dia"), 0xA0: ("a", "acute"),
    0xA1: ("ı", "acute"), 0xA2: ("o", "acute"), 0xA3: ("u", "acute"), 0xA4: ("n", "tilde"),
    0xA5: ("N", "tilde"),
}


def squash(rows, n):
    """Drop repeated rows (the straight parts of a letter) until n remain."""
    rows = list(rows)
    while len(rows) > n:
        i = next(i for i in range(len(rows) - 1) if rows[i] == rows[i + 1])
        del rows[i]
    return rows


def rot180(rows):
    return [r[::-1] for r in reversed(rows)]


def box(arms):
    """Box-drawing glyph from 'UDLR' line weights (0 none, 1 single, 2 double).

    Single lines run on column 2 / row 3, double lines on columns 1,3 / rows
    2,4. A double line is drawn as the outline of a 3-pixel-wide road. A single
    line that ends at a double one stops at its near side; one that crosses it
    runs straight through."""
    u, d, l, r = (int(c) for c in arms)
    v, h = max(u, d), max(l, r)
    road, core = set(), set()
    if h == 2:
        cap_l, cap_r = (1, 3) if v == 2 else (2, 2)
        x0, x1 = (0 if l else cap_l), (4 if r else cap_r)
        road |= {(x, y) for x in range(x0, x1 + 1) for y in (2, 3, 4)}
        core |= {(x, 3) for x in range(0 if l else cap_l + 1, (4 if r else cap_r - 1) + 1)}
    if v == 2:
        cap_t, cap_b = (2, 4) if h == 2 else (3, 3)
        y0, y1 = (0 if u else cap_t), (6 if d else cap_b)
        road |= {(x, y) for x in (1, 2, 3) for y in range(y0, y1 + 1)}
        core |= {(2, y) for y in range(0 if u else cap_t + 1, (6 if d else cap_b - 1) + 1)}
    px = road - core
    if v == 1:
        seg = {(2, y) for y in range(0 if u else 3, (6 if d else 3) + 1)}
        px |= seg if (u and d) else seg - core
    if h == 1:
        seg = {(x, 3) for x in range(0 if l else 2, (4 if r else 2) + 1)}
        px |= seg if (l and r) else seg - core
    return ["".join("#" if (x, y) in px else " " for x in range(W)) for y in range(H)]


BOX = [  # UDLR line weights, in code order from 0xB3
    "1100", "1110", "1120", "2210", "0210", "0120", "2220", "2200", "0220", "2020",
    "2010", "1020", "0110", "1001", "1011", "0111", "1101", "0011", "1111", "1102",
    "2201", "2002", "0202", "2022", "0222", "2202", "0022", "2222", "1022", "2011",
    "0122", "0211", "2001", "1002", "0102", "0201", "2211", "1122", "1010", "0101",
]


def to_bits(row):
    v = 0
    for ch in row:
        v = (v << 1) | (1 if ch == "#" else 0)
    return v


def build():
    """Return ({code: 7 rows}, parsed source glyphs, duplicate keys)."""
    glyphs, dupes = parse_source(SRC_FONT.read_text())
    smiley = glyphs.pop(1)
    glyphs.pop(0, None)  # null terminator

    table = {}
    for key, rows in glyphs.items():
        if not 0x20 <= key <= 0x7E:
            sys.exit(f"unexpected source key {key!r}")
        table[key] = REDRAWN_ASCII.get(chr(key)) or center(rows)
    for ch, rows in EXTRA_ASCII.items():
        assert ord(ch) not in table, ch
        table[ord(ch)] = rows
    table[ord("\\")] = [r[::-1] for r in table[ord("/")]]  # mirror of '/'
    missing = [c for c in range(0x20, 0x7F) if c not in table]
    assert not missing, f"unfilled ASCII: {missing}"

    letters = {chr(k): v for k, v in table.items()}
    letters["ı"] = ["     ", "     "] + letters["i"][2:]
    table[0x01] = smiley
    table.update({c: art(s) for c, s in HAND.items()})
    for i, arms in enumerate(BOX):
        table[0xB3 + i] = box(arms)
    for code, (base, accent) in ACCENTED.items():
        body = squash(letters[base], 5) if base.isupper() else letters[base][2:]
        table[code] = art(ACCENTS[accent] + "|" + "|".join(body))
    table[0x80] = squash(letters["C"], 6) + ["  #  "]  # Ç
    table[0x87] = letters["c"][1:] + ["  #  "]         # ç
    table[0xA8] = rot180(letters["?"])                 # ¿
    table[0xAD] = rot180(letters["!"])                 # ¡
    table[0xF5] = rot180(table[0xF4])                  # ⌡

    for code, rows in table.items():
        assert len(rows) == H and all(len(r) == W for r in rows), hex(code)
    unfilled = [hex(c) for c in range(0x01, 0xFF) if c not in table]
    assert not unfilled, f"unfilled codes: {unfilled}"
    return table, glyphs, dupes


def main():
    table, glyphs, dupes = build()

    # ---- titrm_font.c --------------------------------------------------
    lines = [
        "/* GENERATED by tools/gen_font.py from petabyt/font (MIT). Do not edit. */",
        "#include \"titrm_font.h\"",
        "",
        "/* one byte per row, bits 4..0 = pixels left to right */",
        "const term_glyph_t term_font[256] = {",
    ]
    for code in sorted(table):
        rows = ", ".join(f"0x{to_bits(r):02X}" for r in table[code])
        if code in NAMES:
            label = NAMES[code]
        elif code == ord("\\"):
            label = "backslash"  # a literal '\' at end of a comment would splice lines
        elif code == 0x20:
            label = "space"
        elif 0x20 < code < 0x7F:
            label = chr(code)
        else:
            label = f"U+{ord(CP437[code]):04X}"  # keep the file ASCII
        lines.append(f"    [0x{code:02X}] = {{{{ {rows} }}}}, /* {label} */")
    lines += ["};", ""]
    OUT_C.write_text("\n".join(lines), newline="\n")

    # ---- titrm_chars.h -------------------------------------------------
    by_name = sorted(NAMES, key=lambda c: NAMES[c])
    h = [
        "/* GENERATED by tools/gen_font.py. Do not edit. */",
        "#ifndef TITRM_CHARS_H",
        "#define TITRM_CHARS_H",
        "",
        "/* Codes 0x01-0x1F and 0x7F-0xFF follow code page 437 (see FONT.md for the",
        " * full table); these are the glyphs titrmlib names. */",
    ]
    for code in by_name:
        h.append(f"#define TERM_CH_{NAMES[code]:<10} 0x{code:02X}")
    h += [
        "",
        "/* Box drawing and blocks: stretched over the gaps between cells. */",
        f"#define TERM_CH_CONNECTED_FIRST 0x{CONNECTED_FIRST:02X}",
        f"#define TERM_CH_CONNECTED_LAST  0x{CONNECTED_LAST:02X}",
        "",
        "/* The same glyphs as string literals, for building text:",
        " *   TERM_S_SIG3 \"HomeWiFi\"  ->  \"\\x16\" \"HomeWiFi\"",
        " * (adjacent literals concatenate after escapes are processed, so a",
        " * following hex digit can't be swallowed into the escape.) */",
    ]
    for code in by_name:
        h.append(f"#define TERM_S_{NAMES[code]:<10} \"\\x{code:02X}\"")
    h += ["", "#endif", ""]
    OUT_H.write_text("\n".join(h), newline="\n")

    # ---- FONT.md -------------------------------------------------------
    md = [
        "# titrmlib font map",
        "",
        "Generated by `tools/gen_font.py`. ASCII comes from"
        " [petabyt/font](https://github.com/petabyt/font) (MIT, `tools/petabyt-font/LICENSE`);"
        " everything else is drawn in the script. 5x7 glyphs, packed one byte per row.",
        "",
        "Fixes applied to the source font (see the script for detail):",
        "",
        "- `-` and `_` were each defined twice; kept the later `-` (5px, row 4).",
        "- Raw code `1` was a smiley face; it is CP437's `☺` at 0x01 (`TERM_CH_SMILE`).",
        "- Missing `$ & @ [ ] \\ ^ |` drawn by hand (`\\` mirrors `/`).",
        "- `I i l 1` redrawn with serifs to fill the cell; other narrow glyphs"
        " (`! . ' : ( )` etc.) centered instead of flush-left, so they don't leave a gap.",
        "- Source's all-filled null terminator dropped (`TERM_CH_BLOCK` is the same idea).",
        "",
        "## 0x20-0x7E: ASCII",
        "",
        "All 95 printable characters; standard ASCII positions.",
        "",
        "## 0x01-0x1F, 0x7F-0xFF: code page 437",
        "",
        "The IBM PC character set, except that 0x13-0x16 (`‼ ¶ § ▬` in CP437) hold the",
        "signal-strength icons. 0x00 and 0xFF are blank. 0x00 ends a string, and",
        "`term_panel_print` treats 0x0A and 0x0D as newline and carriage return, so",
        "those three can only be placed with `term_put`.",
        "",
        f"0x{CONNECTED_FIRST:02X}-0x{CONNECTED_LAST:02X} (box drawing and blocks) are *connected*:"
        " the renderer extends them across the 1px column/row gap between cells so they join.",
        "",
        "| Code | CP437 | Name | Glyph |",
        "|------|-------|------|-------|",
    ]
    for code in list(range(0x01, 0x20)) + list(range(0x7F, 0xFF)):
        uni = "signal" if 0x13 <= code <= 0x16 else CP437[code]
        name = f"`TERM_CH_{NAMES[code]}`" if code in NAMES else ""
        md.append(f"| 0x{code:02X} | {uni} | {name} | "
                  f"<pre>{'<br>'.join(table[code]).replace(' ', '.')}</pre> |")
    md += ["", "The `.` in the art above is an empty pixel.", ""]
    OUT_MD.write_text("\n".join(md), newline="\n", encoding="utf-8")

    print(f"parsed {len(glyphs)} source glyphs; duplicates resolved: "
          f"{sorted(chr(d) for d in set(dupes))}")
    print(f"wrote {OUT_C.name}, {OUT_H.name}, {OUT_MD.name}")


if __name__ == "__main__":
    main()
