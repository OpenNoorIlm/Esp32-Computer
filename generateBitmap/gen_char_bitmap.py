#!/usr/bin/env python3
"""
gen_char_bitmap.py  –  Generate char.bitmap for MiniPC (ESP32 / SSD1306)

Paths (all relative to this script's location):
  fonts/Press_Start_2P/PressStart2P-Regular.ttf   → English  (ASCII 0x20-0x7E)
  fonts/Amiri/Amiri-Regular.ttf                   → Arabic   (core + shaped forms)
  fonts/Noto_Nastaliq_Urdu/static/
        NotoNastaliqUrdu-Regular.ttf               → Urdu extras (Arabic Supplement)
  Output → ..\SD_CARD\char.bitmap

Binary format (loadFont() in 01_display.ino / config.h):
  [0:4]   magic  "CMAP"
  [4:7]   reserved  0x00 0x00 0x00
  [7:9]   count  uint16 LE
  per glyph (12 bytes each):
    [+0:4]  codepoint  uint32 LE
    [+4:12] bitmap     8 bytes  (one per row, MSB = leftmost pixel, CHAR_W=8)

Codepoint budget  (MAX_CHARS = 300):
  English printable ASCII  0x0020–0x007E  =  95
  Arabic letters+harakat   0x0621–0x0652  =  50    ← Amiri
  Arabic-Indic digits      0x0660–0x0669  =  10    ← Amiri
  Pres.Forms-B (shaped)    0xFE70–0xFEFF  = 144    ← Amiri
  ─────────────────────────────────────────────
  Total                                   = 299  ✓
"""

import struct, sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

try:
    import arabic_reshaper
    from bidi.algorithm import get_display
    ARABIC_SHAPING = True
except ImportError:
    ARABIC_SHAPING = False
    print("Warning: arabic_reshaper / python-bidi not found.\n"
          "  pip install arabic-reshaper python-bidi\n"
          "Arabic glyphs will render without contextual shaping.", file=sys.stderr)

# ── Paths ────────────────────────────────────────────────────────────────────
HERE     = Path(__file__).resolve().parent
FONTS    = HERE / "fonts"
OUT_DIR  = HERE.parent / "SD_CARD"
OUT_FILE = OUT_DIR / "char.bitmap"

FONT_EN  = FONTS / "Press_Start_2P"  / "PressStart2P-Regular.ttf"
FONT_AR  = FONTS / "Amiri"           / "Amiri-Regular.ttf"
FONT_UR  = FONTS / "Noto_Nastaliq_Urdu" / "static" / "NotoNastaliqUrdu-Regular.ttf"

# ── Constants (must match config.h) ─────────────────────────────────────────
CHAR_W    = 8
CHAR_H    = 8
MAX_CHARS = 300
MAGIC     = b"CMAP"

# ── Codepoint groups ─────────────────────────────────────────────────────────
CP_ENGLISH = list(range(0x0020, 0x007F))            # 95  printable ASCII
CP_AR_CORE = list(range(0x0621, 0x0653))            # 50  ء–ْ  letters + harakat
CP_AR_DIGS = list(range(0x0660, 0x066A))            # 10  ٠–٩  Eastern-Arabic digits
CP_AR_PRES = list(range(0xFE70, 0xFF00))            # 144 shaped presentation forms-B

# ── Renderer ─────────────────────────────────────────────────────────────────
def make_image() -> Image.Image:
    return Image.new("L", (CHAR_W, CHAR_H), 0)

def render_glyph(cp: int, font: ImageFont.FreeTypeFont, is_rtl: bool = False) -> list[int]:
    """Return 8 row-bytes for codepoint cp using the given font."""
    ch = chr(cp)

    # Apply Arabic shaping so we get the correct contextual/isolated form
    if is_rtl and ARABIC_SHAPING:
        try:
            ch = get_display(arabic_reshaper.reshape(ch))
        except Exception:
            pass

    img = make_image()
    draw = ImageDraw.Draw(img)

    try:
        bb = font.getbbox(ch)
        gw, gh = bb[2] - bb[0], bb[3] - bb[1]
        ox = max(0, (CHAR_W - gw) // 2) - bb[0]
        oy = max(0, (CHAR_H - gh) // 2) - bb[1]
    except Exception:
        ox, oy = 0, 0

    draw.text((ox, oy), ch, fill=255, font=font)

    px = img.load()
    rows = []
    for row in range(CHAR_H):
        byte = 0
        for col in range(CHAR_W):
            if px[col, row] > 48:
                byte |= (1 << (7 - col))
        rows.append(byte)
    return rows

def load_ttf(path: Path, size: int) -> ImageFont.FreeTypeFont:
    if not path.exists():
        sys.exit(f"Font not found: {path}")
    return ImageFont.truetype(str(path), size)

# ── Build glyph table ─────────────────────────────────────────────────────────
def build_glyphs() -> dict[int, list[int]]:
    print("Loading fonts…")
    fnt_en = load_ttf(FONT_EN, 8)
    fnt_ar = load_ttf(FONT_AR, 8)
    # Urdu font loaded but used only as fallback for extended Arabic Supplement
    # (all 299 slots are already filled by EN+AR; kept here for easy future use)

    glyphs: dict[int, list[int]] = {}

    print(f"  Rendering {len(CP_ENGLISH)} English glyphs  (PressStart2P)…")
    for cp in CP_ENGLISH:
        glyphs[cp] = render_glyph(cp, fnt_en, is_rtl=False)

    ar_cps = CP_AR_CORE + CP_AR_DIGS + CP_AR_PRES
    print(f"  Rendering {len(ar_cps)} Arabic glyphs  (Amiri)…")
    for cp in ar_cps:
        glyphs[cp] = render_glyph(cp, fnt_ar, is_rtl=True)

    total = len(glyphs)
    print(f"  Total: {total} glyphs  (MAX_CHARS={MAX_CHARS})")
    if total > MAX_CHARS:
        print(f"  ⚠ Truncating to {MAX_CHARS}", file=sys.stderr)

    return glyphs

# ── Writer ────────────────────────────────────────────────────────────────────
def write_bitmap(glyphs: dict[int, list[int]]):
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    entries = list(glyphs.items())[:MAX_CHARS]
    count   = len(entries)
    size    = 9 + count * 12

    with open(OUT_FILE, "wb") as f:
        f.write(MAGIC)                       # 4 bytes
        f.write(b'\x00\x00\x00')            # 3 bytes reserved
        f.write(struct.pack('<H', count))    # 2 bytes count LE
        for cp, rows in entries:
            f.write(struct.pack('<I', cp))   # 4 bytes codepoint LE
            for b in rows[:CHAR_H]:
                f.write(struct.pack('B', b)) # 1 byte per row

    print(f"\n✓  {OUT_FILE}")
    print(f"   {count} glyphs · {size} bytes")

# ── Preview (ASCII art) ───────────────────────────────────────────────────────
def preview(glyphs: dict[int, list[int]], sample: str = "Hello MiniPC! 0123"):
    print("\nPreview:")
    for row in range(CHAR_H):
        line = ""
        for ch in sample:
            cp = ord(ch)
            if cp in glyphs:
                b = glyphs[cp][row]
                line += "".join("█" if b & (1 << (7-c)) else " " for c in range(CHAR_W))
            else:
                line += "?" * CHAR_W
            line += " "
        print(" " + line)

# ── Main ──────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    print("=" * 52)
    print("  gen_char_bitmap.py  –  MiniPC font builder")
    print("=" * 52)
    glyphs = build_glyphs()
    write_bitmap(glyphs)
    preview(glyphs)