#!/usr/bin/env python3

import json
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

TILES_DIR = ROOT / "assets" / "tiles"

SHEET_WIDTH = 96
SHEET_HEIGHT = 80

TILE = 16

RIGHT = 64

SPECIAL_ROW = 64

VARIANT_SHIFTS = ((3, 0), (0, 3), (2, 5), (5, 2), (1, 6), (6, 1), (4, 4))

INK = (0xD6, 0xE0, 0xD8, 255)
TRANSPARENT = (0, 0, 0, 0)

TERRAINS = ("floor", "wall", "water", "cliff", "path", "stair")

SLOTS = {
    "surface_variant_1": [64, 0],
    "surface_variant_2": [80, 0],
    "surface_variant_3": [64, 16],
    "surface_variant_4": [80, 16],
    "surface_variant_5": [64, 32],
    "surface_variant_6": [80, 32],
    "surface_variant_7": [64, 48],
    "water_frame_b": [80, 48],
    "wall_band": [0, 64],
    "wall_rim": [8, 64],
    "bridge_deck": [16, 64],
    "shade": [24, 64],
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
    fx = x / 15.0
    fy = y / 15.0

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


def on_pipe(at: int) -> bool:
    return at in (7, 8)


def pipe_ink(piece: int, x: int, y: int) -> bool:
    if piece == 0:
        return on_pipe(x) or on_pipe(y)

    if piece == 1:
        return on_pipe(y)

    if piece == 2:
        return on_pipe(x)

    if piece == 3:
        return (on_pipe(x) and y <= 8) or (on_pipe(y) and x >= 7)

    if piece == 4:
        return (on_pipe(x) and y >= 7) or (on_pipe(y) and x <= 8)

    if piece == 5:
        return on_pipe(y) or (on_pipe(x) and y >= 7)

    if piece == 6:
        return on_pipe(y) or (
            5 <= x <= 10
            and 5 <= y <= 10
            and (x in (5, 10) or y in (5, 10))
        )

    return (
        (3 <= x <= 12 and 3 <= y <= 12 and (x in (3, 12) or y in (3, 12)))
        or (on_pipe(x) and (y < 3 or y > 12))
        or (on_pipe(y) and (x < 3 or x > 12))
        or (y in (6, 9) and 5 <= x <= 10)
    )


def wall_backdrop_ink(x: int, y: int) -> bool:
    return x % 4 == 2 and y % 4 == 2


def wall_interior_ink(piece: int, x: int, y: int) -> bool:
    return wall_backdrop_ink(x, y) or pipe_ink(piece, x, y)


def floor_detail_ink(variant: int, x: int, y: int) -> bool:
    if variant == 1:
        return y == 8 and x % 3 != 2

    if variant == 2:
        return x == 8 and y % 3 != 2

    if variant == 3:
        return x in (2, 13) and y in (2, 13)

    if variant == 4:
        return 5 <= x <= 10 and 5 <= y <= 10 and (x + y) % 2 == 0

    if variant == 5:
        return (x, y) in ((3, 10), (11, 4), (12, 12))

    if variant == 6:
        return (x == 8 and y <= 8 and y % 3 != 2) or (
            y == 8 and x <= 8 and x % 3 != 2
        )

    return 6 <= x <= 9 and 6 <= y <= 9 and (x in (6, 9) or y in (6, 9))


def surface_ink(terrain: str, mask: int, x: int, y: int) -> bool:
    if mask == 15 and terrain == "wall":
        return wall_interior_ink(0, x, y)

    value = coverage(mask, x, y)

    if value < 0.5:
        return False

    if value < 0.66:
        return True

    return pattern_ink(terrain, x % 8, y % 8)


def variant_ink(terrain: str, variant: int, x: int, y: int) -> bool:
    if terrain == "wall":
        return wall_interior_ink(variant, x, y)

    if terrain == "floor":
        return pattern_ink(terrain, x % 8, y % 8) or floor_detail_ink(
            variant, x, y
        )

    shift_x, shift_y = VARIANT_SHIFTS[variant - 1]

    return pattern_ink(terrain, (x + shift_x) % 8, (y + shift_y) % 8)


def band_ink(x: int, y: int) -> bool:
    return x % 2 == 0 or y % 4 == 0


def rim_ink(x: int, y: int) -> bool:
    return y < 2 or x % 4 == 0


def bridge_ink(x: int, y: int) -> bool:
    return y % 4 != 0 and (x + 2 * (y // 4)) % 8 != 7


def shade_ink(x: int, y: int) -> bool:
    return (x + y) % 2 == 0


def frame_b_ink(terrain: str, x: int, y: int) -> bool:
    return pattern_ink(terrain, (x + 2) % 8, y % 8)


def sheet_pixels(terrain: str) -> bytearray:
    pixels = bytearray(SHEET_WIDTH * SHEET_HEIGHT * 4)

    def put(x: int, y: int) -> None:
        at = (y * SHEET_WIDTH + x) * 4
        pixels[at : at + 4] = bytes(INK)

    for mask in range(16):
        origin_x = (mask % 4) * TILE
        origin_y = (mask // 4) * TILE

        for y in range(TILE):
            for x in range(TILE):
                if surface_ink(terrain, mask, x, y):
                    put(origin_x + x, origin_y + y)

    for variant in range(1, 8):
        slot = variant - 1
        origin_x = RIGHT + slot % 2 * TILE
        origin_y = slot // 2 * TILE

        for y in range(TILE):
            for x in range(TILE):
                if variant_ink(terrain, variant, x, y):
                    put(origin_x + x, origin_y + y)

    for y in range(TILE):
        for x in range(TILE):
            if frame_b_ink(terrain, x, y):
                put(RIGHT + TILE + x, 3 * TILE + y)

    for y in range(8):
        for x in range(8):
            if band_ink(x, y):
                put(x, SPECIAL_ROW + y)

            if rim_ink(x, y):
                put(8 + x, SPECIAL_ROW + y)

            if bridge_ink(x, y):
                put(16 + x, SPECIAL_ROW + y)

            if shade_ink(x, y):
                put(24 + x, SPECIAL_ROW + y)

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
        "masks": "4x4 of 16x16 at 0,0",
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
