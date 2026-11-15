"""
Makes extra colour themes for the controller from an existing one (fork).

A theme is a heatshrink-compressed archive in the SPIFFS image (spiffs_image/t_<n>.sz.hs):
    uint32 count, then per file: name (NUL-terminated) + uint32 size, then the files' bytes in that order.
It holds the theme's artwork (LVGL images and GIF animations). This tool unpacks the Main theme, recolours every
image from Main's palette to a new one, and packs the result as a new archive.

    python tools/theme_tool.py list spiffs_image/t_1.sz.hs
    python tools/theme_tool.py build            # writes the themes in THEMES below to spiffs_image/

Heatshrink (window 2^14, lookahead 2^7 - CompressedFile.cpp): a bit stream, MSB first; 1 + 8 bits = a literal
byte, 0 + 14 bits (offset-1) + 7 bits (count-1) = copy count bytes from offset bytes back.
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

WINDOW, LOOKAHEAD = 14, 7
ROOT = Path(__file__).resolve().parent.parent


# --- heatshrink ------------------------------------------------------------------------------------------------
def hs_decode(data: bytes) -> bytes:
    out = bytearray()
    bitpos, nbits = 0, len(data) * 8

    def bits(n):
        nonlocal bitpos
        if bitpos + n > nbits:
            raise EOFError
        v = 0
        for _ in range(n):
            v = (v << 1) | ((data[bitpos >> 3] >> (7 - (bitpos & 7))) & 1)
            bitpos += 1
        return v
    try:
        while True:
            if bits(1):
                out.append(bits(8))
            else:
                off = bits(WINDOW) + 1
                cnt = bits(LOOKAHEAD) + 1
                for _ in range(cnt):
                    out.append(out[-off] if off <= len(out) else 0)  # the window starts zero-filled
    except EOFError:
        pass
    return bytes(out)


def hs_encode(data: bytes) -> bytes:
    """LZSS with hash chains and one-step lazy matching; a back-reference (22 bits) beats literals (9 bits each)
    from 3 bytes."""
    win, maxlen = 1 << WINDOW, 1 << LOOKAHEAD
    out, acc, nacc = bytearray(), 0, 0

    def put(v, n):
        nonlocal acc, nacc
        for i in range(n - 1, -1, -1):
            acc = (acc << 1) | ((v >> i) & 1)
            nacc += 1
            if nacc == 8:
                out.append(acc)
                acc, nacc = 0, 0
    heads: dict[bytes, list[int]] = {}
    n = len(data)

    def longest(i):
        best_len, best_off = 0, 0
        if i + 3 > n:
            return 0, 0
        for j in reversed(heads.get(data[i:i + 3], [])[-256:]):
            if i - j > win:
                break
            k = 3
            while k < maxlen and i + k < n and data[j + k] == data[i + k]:
                k += 1
            if k > best_len:
                best_len, best_off = k, i - j
                if k == maxlen:
                    break
        return best_len, best_off

    def add(p):
        if p + 3 <= n:
            heads.setdefault(data[p:p + 3], []).append(p)
    i = 0
    while i < n:
        length, off = longest(i)
        if length >= 3 and length < maxlen:
            add(i)
            nxt, _ = longest(i + 1)  # lazy: a longer match one byte later beats this one plus a literal
            heads[data[i:i + 3]].pop()
            if nxt > length + 1:
                length = 0
        if length >= 3:
            put(0, 1)
            put(off - 1, WINDOW)
            put(length - 1, LOOKAHEAD)
            step = length
        else:
            put(1, 1)
            put(data[i], 8)
            step = 1
        for p in range(i, i + step):
            add(p)
        i += step
    if nacc:
        out.append(acc << (8 - nacc))
    return bytes(out)


# --- archive ---------------------------------------------------------------------------------------------------
def unpack(raw: bytes) -> list[tuple[str, bytes]]:
    count = struct.unpack_from("<I", raw, 0)[0]
    pos, entries = 4, []
    for _ in range(count):
        end = raw.index(b"\0", pos)
        name = raw[pos:end].decode()
        size = struct.unpack_from("<I", raw, end + 1)[0]
        entries.append((name, size))
        pos = end + 5
    pos += 1  # the directory's terminator byte (ArchiveCache reads it)
    files = []
    for name, size in entries:
        files.append((name, raw[pos:pos + size]))
        pos += size
    return files


def pack(files: list[tuple[str, bytes]]) -> bytes:
    out = bytearray(struct.pack("<I", len(files)))
    for name, data in files:
        out += name.encode() + b"\0" + struct.pack("<I", len(data))
    out += b"\0"  # directory terminator
    for _, data in files:
        out += data
    return bytes(out)



# --- recolouring -----------------------------------------------------------------------------------------------
# Main's palette (ThemeService.cpp): primary, secondary, background
MAIN = ((160, 250, 0), (0, 180, 0), (0, 0, 0))

# New themes: archive number, name, primary, secondary, background. Keep in step with ThemeService.cpp / Themes.hpp.
THEMES = [
    (5, "Portal", (0, 230, 190), (120, 255, 60), (0, 0, 0)),
    (6, "Butter", (255, 214, 64), (214, 140, 0), (30, 18, 0)),
    (7, "Ice", (120, 230, 255), (0, 140, 255), (0, 10, 40)),
    (8, "Mono", (255, 255, 255), (150, 150, 150), (0, 0, 0)),
    (9, "Purple", (200, 120, 255), (255, 80, 200), (16, 0, 32)),
]


def map_color(c, new):
    """Express c as background + a*primary + b*secondary + w*white in Main's colours, then rebuild it from the new
    theme's. Colours that aren't made of Main's (photos, the odd red or blue detail) come out unchanged."""
    (P, S, B), (P2, S2, B2) = MAIN, new
    mix = lambda x, y, t: tuple(round(x[i] + (y[i] - x[i]) * t) for i in range(3))  # noqa: E731
    # Main's two extra shades: a light highlight (primary towards white) and a very dark green (secondary in shadow)
    for known, target in (((235, 255, 47), mix(P2, (255, 255, 255), 0.45)), ((0, 50, 5), mix(B2, S2, 0.28))):
        if max(abs(c[i] - known[i]) for i in range(3)) <= 12:
            return target
    r, g, b = c
    w = b / 255.0                       # only white has blue in Main's palette
    a = (r - 255 * w) / 160.0           # only the primary has red
    bb = (g - 255 * w - 250 * a) / 180.0
    rebuilt = [P[i] * a + S[i] * bb + 255 * w for i in range(3)]
    if max(abs(rebuilt[i] - c[i]) for i in range(3)) > 24 or a < -0.05 or bb < -0.05 or a > 1.3 or bb > 1.3:
        return c
    a, bb = max(0.0, a), max(0.0, bb)
    rest = max(0.0, 1.0 - a - bb - w)   # how much of the background shows through
    out = [B2[i] * rest + P2[i] * a + S2[i] * bb + 255 * w for i in range(3)]
    return tuple(max(0, min(255, round(v))) for v in out)


def palette_info(d: bytes):
    """(entry count, palette offset) for LVGL indexed images (I1/I2/I4/I8), else None."""
    if len(d) > 12 and d[0] == 0x19 and d[1] in (7, 8, 9, 10):
        return 1 << (1 << (d[1] - 7)), 12
    return None


def recolor_image(d: bytes, new) -> bytes:
    info = palette_info(d)
    if info is None:
        return d
    n, off = info
    out = bytearray(d)
    for i in range(n):
        b, g, r, a = d[off + 4 * i: off + 4 * i + 4]
        if a == 0:
            continue
        r2, g2, b2 = map_color((r, g, b), new)
        out[off + 4 * i: off + 4 * i + 3] = bytes((b2, g2, r2))
    return bytes(out)


def to_png(d: bytes, path: str, scale: int = 3):
    """Preview an indexed LVGL image (dev aid)."""
    from PIL import Image
    info = palette_info(d)
    if info is None:
        return False
    n, off = info
    bpp = 1 << (d[1] - 7)
    w, h, stride = struct.unpack_from("<HHH", d, 4)
    pal = [tuple(d[off + 4 * i + k] for k in (2, 1, 0, 3)) for i in range(n)]
    px = d[off + 4 * n:]
    img = Image.new("RGBA", (w, h))
    for y in range(h):
        for x in range(w):
            bit = x * bpp
            byte = px[y * stride + bit // 8] if y * stride + bit // 8 < len(px) else 0
            idx = (byte >> (8 - bpp - bit % 8)) & (n - 1)
            img.putpixel((x, y), pal[idx])
    img.resize((w * scale, h * scale), Image.NEAREST).save(path)
    return True


def build(src: Path, out_dir: Path):
    raw = hs_decode(src.read_bytes())
    files = unpack(raw)
    for num, name, p, s_, b in THEMES:
        new_files = [(fname, recolor_image(data, (p, s_, b))) for fname, data in files]
        packed = pack(new_files)
        enc = hs_encode(packed)
        assert hs_decode(enc)[:len(packed)] == packed
        dest = out_dir / f"t_{num}.sz.hs"
        dest.write_bytes(enc)
        print(f"{dest.name}: {name}, {len(enc)} bytes")

def main():
    cmd = sys.argv[1] if len(sys.argv) > 1 else "build"
    if cmd == "list":
        raw = hs_decode(Path(sys.argv[2]).read_bytes())
        for name, data in unpack(raw):
            print(f"{name:32s} {len(data):7d}  {data[:16].hex()}")
        print(f"{len(raw)} bytes unpacked")
        return
    if cmd == "build":
        build(ROOT / "spiffs_image" / "t_1.sz.hs", ROOT / "spiffs_image")
        return
    if cmd == "roundtrip":
        src = Path(sys.argv[2]).read_bytes()
        raw = hs_decode(src)
        enc = hs_encode(raw)
        assert hs_decode(enc)[:len(raw)] == raw
        assert pack(unpack(raw)) == raw
        print(f"ok: {len(src)} -> {len(raw)} -> {len(enc)}")
        return


if __name__ == "__main__":
    main()
