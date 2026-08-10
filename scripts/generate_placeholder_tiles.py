#!/usr/bin/env python3

import json
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

TILES_DIR = ROOT / "assets" / "tiles"

SHEET_WIDTH = 32
SHEET_HEIGHT = 48

INK = (0xD6, 0xE0, 0xD8, 255)
TRANSPARENT = (0, 0, 0, 0)

TERRAINS = ("floor", "wall", "water", "cliff", "path", "stair")

SLOTS = {
    "wall_band": [0, 32],
    "wall_rim": [8, 32],
    "bridge_deck": [16, 32],
    "shade": [24, 32],
    "surface_variant_1": [0, 40],
    "surface_variant_2": [8, 40],
    "water_frame_b": [16, 40],
    "spare": [24, 40],
}


def pattern_ink(terrain: str, x: int, y: int) -> bool:
    if terrain == "floor":
        return (x == 3 and y == 5) or (x == 6 and y == 1)

    if terrain == "wall":
        return (x + y) % 2 == 0

    if terrain == "water":
        return y % 4 == 2 and x % 4 != 3

    if terrain == "cliff":
        return x % 3 != 1

    if terrain == "path":
        return (x + y) % 4 == 0

    return y % 2 == 0


def coverage(mask: int, x: int, y: int) -> float:
    fx = x / 7.0
    fy = y / 7.0

    value = 0.0

    if mask & 1:
        value += (1.0 - fx) * (1.0 - fy)

    if mask & 2:
        value += fx * (1.0 - fy)

    if mask & 4:
        value += (1.0 - fx) * fy

    if mask & 8:
        value += fx * fy

    return value


def surface_ink(terrain: str, mask: int, x: int, y: int) -> bool:
    value = coverage(mask, x, y)

    if value < 0.5:
        return False

    if value < 0.66:
        return True

    return pattern_ink(terrain, x, y)


def band_ink(x: int, y: int) -> bool:
    return x % 2 == 0 or y % 4 == 0


def rim_ink(x: int, y: int) -> bool:
    return y < 2 or x % 4 == 0


def bridge_ink(x: int, y: int) -> bool:
    return y % 4 != 0 and (x + 2 * (y // 4)) % 8 != 7


def shade_ink(x: int, y: int) -> bool:
    return (x + y) % 2 == 0


def variant_one_ink(terrain: str, x: int, y: int) -> bool:
    return pattern_ink(terrain, (x + 3) % 8, y)


def variant_two_ink(terrain: str, x: int, y: int) -> bool:
    return pattern_ink(terrain, x, (y + 3) % 8)


def frame_b_ink(terrain: str, x: int, y: int) -> bool:
    return pattern_ink(terrain, (x + 2) % 8, y)


def sheet_pixels(terrain: str) -> bytearray:
    pixels = bytearray(SHEET_WIDTH * SHEET_HEIGHT * 4)

    def put(x: int, y: int) -> None:
        at = (y * SHEET_WIDTH + x) * 4
        pixels[at : at + 4] = bytes(INK)

    for mask in range(16):
        origin_x = (mask % 4) * 8
        origin_y = (mask // 4) * 8

        for y in range(8):
            for x in range(8):
                if surface_ink(terrain, mask, x, y):
                    put(origin_x + x, origin_y + y)

    for y in range(8):
        for x in range(8):
            if band_ink(x, y):
                put(x, 32 + y)

            if rim_ink(x, y):
                put(8 + x, 32 + y)

            if bridge_ink(x, y):
                put(16 + x, 32 + y)

            if shade_ink(x, y):
                put(24 + x, 32 + y)

            if variant_one_ink(terrain, x, y):
                put(x, 40 + y)

            if variant_two_ink(terrain, x, y):
                put(8 + x, 40 + y)

            if frame_b_ink(terrain, x, y):
                put(16 + x, 40 + y)

    return pixels


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + kind
        + payload
        + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)
    )


def png_bytes(width: int, height: int, pixels: bytes) -> bytes:
    raw = bytearray()

    for row in range(height):
        raw.append(0)
        raw += pixels[row * width * 4 : (row + 1) * width * 4]

    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)

    return (
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", header)
        + png_chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + png_chunk(b"IEND", b"")
    )


def sidecar_text() -> str:
    sidecar = {
        "sheet": {"width": SHEET_WIDTH, "height": SHEET_HEIGHT},
        "masks": "4x4 at 0,0",
        "slots": SLOTS,
    }

    return json.dumps(sidecar, indent=2) + "\n"


def main() -> int:
    TILES_DIR.mkdir(parents=True, exist_ok=True)

    for terrain in TERRAINS:
        target = TILES_DIR / f"{terrain}.png"
        target.write_bytes(
            png_bytes(
                SHEET_WIDTH,
                SHEET_HEIGHT,
                bytes(sheet_pixels(terrain)),
            )
        )
        print(f"OK: wrote {target}")

    sidecar = TILES_DIR / "tiles.json"
    sidecar.write_text(sidecar_text(), encoding="utf-8")
    print(f"OK: wrote {sidecar}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
