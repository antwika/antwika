#!/usr/bin/env python3

import argparse
import re
import struct
import sys
import zlib
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent

TEXTURES_DIR = Path("assets/textures")

COLUMNS = 16

ROWS = 16

PADDING_BETWEEN_TILES = 2

TILE_SHAPES = ((15, 9), (15, 12))

Rgba = tuple[int, int, int, int]

BLANK: Rgba = (0, 39, 43, 255)


def atlas_size(tile_width: int, tile_height: int) -> tuple[int, int]:
    width = (
        COLUMNS * tile_width
        + (COLUMNS - 1) * PADDING_BETWEEN_TILES
    )
    height = ROWS * tile_height + (ROWS - 1) * PADDING_BETWEEN_TILES

    return (width, height)


def tile_pixel() -> Rgba:
    return BLANK


def atlas_pixels(tile_width: int, tile_height: int) -> bytes:
    width, height = atlas_size(tile_width, tile_height)
    pixels = bytearray(width * height * 4)

    for row in range(ROWS):
        top = row * (tile_height + PADDING_BETWEEN_TILES)

        for column in range(COLUMNS):
            left = column * (tile_width + PADDING_BETWEEN_TILES)

            for y in range(tile_height):
                for x in range(tile_width):
                    at = ((top + y) * width + left + x) * 4
                    pixels[at : at + 4] = bytes(tile_pixel())

    return bytes(pixels)


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


def atlas_bytes(tile_width: int, tile_height: int) -> bytes:
    width, height = atlas_size(tile_width, tile_height)

    return png_bytes(
        width, height, atlas_pixels(tile_width, tile_height)
    )


def atlas_name(tile_width: int, tile_height: int) -> str:
    return f"atlas-{tile_width}x{tile_height}.png"


PALETTE_HEADER = Path(
    "src/apps/game/include/antwika/game/TilePaint.hpp"
)

FIRST_COLOUR = re.compile(
    r"kPalette\{\s*gfx::Color\{"
    r"\.red = (\d+), \.green = (\d+), \.blue = (\d+)"
)


def palette_first(root: Path) -> Rgba | None:
    path = root / PALETTE_HEADER

    if not path.exists():
        return None

    found = FIRST_COLOUR.search(path.read_text())

    if found is None:
        return None

    return (
        int(found.group(1)),
        int(found.group(2)),
        int(found.group(3)),
        255,
    )


def stale_atlases(directory: Path) -> list[str]:
    out: list[str] = []

    for tile_width, tile_height in TILE_SHAPES:
        path = directory / atlas_name(tile_width, tile_height)
        held = path.read_bytes() if path.exists() else b""

        if held != atlas_bytes(tile_width, tile_height):
            out.append(str(path))

    return out


def write_atlases(directory: Path) -> None:
    directory.mkdir(parents=True, exist_ok=True)

    for tile_width, tile_height in TILE_SHAPES:
        path = directory / atlas_name(tile_width, tile_height)

        path.write_bytes(atlas_bytes(tile_width, tile_height))

        width, height = atlas_size(tile_width, tile_height)

        print(f"{path}: {width}x{height}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Write the blank texture atlases."
    )
    parser.add_argument("--root", type=Path, default=DEFAULT_ROOT)
    parser.add_argument(
        "--check",
        action="store_true",
        help="Report whether the written atlases are current.",
    )
    arguments = parser.parse_args()

    directory = arguments.root / TEXTURES_DIR

    if not arguments.check:
        write_atlases(directory)

        return 0

    stale = stale_atlases(directory)

    for name in stale:
        print(f"{name} is out of date", file=sys.stderr)

    first = palette_first(arguments.root)

    if first is not None and first != BLANK:
        print(
            f"the atlases are blank in {BLANK} but the palette "
            f"begins with {first}",
            file=sys.stderr,
        )

        return 1

    return 1 if stale else 0


if __name__ == "__main__":
    sys.exit(main())
