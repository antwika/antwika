#!/usr/bin/env python3
# Draws the texture atlas apps/game renders its grid from.
# The art is generated rather than painted.
# So a tile's shape comes from the projection the game draws it with.
# The slot layout here is the one antwika/game/TileAtlas.hpp addresses.
import argparse
import struct
import sys
import zlib
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent

ATLAS_PATH = Path("src/apps/game/assets/atlas.png")

TILE_WIDTH = 128
TILE_HEIGHT = 64
COLUMNS = 8
ROWS = 3

GROUND_SLOT = 0
FIRST_ROAD_SLOT = 1
ROAD_SLOT_COUNT = 16
FIRST_WALKER_SLOT = 17
WALKER_SLOT_COUNT = 4

LINK_NORTH = 1 << 0
LINK_EAST = 1 << 1
LINK_SOUTH = 1 << 2
LINK_WEST = 1 << 3

# Half the width of a road, as a fraction of a cell.
ROAD_HALF = 0.17

# How far in from a diamond's edge its darker rim reaches.
RIM = 0.035

Rgba = tuple[int, int, int, int]

TRANSPARENT: Rgba = (0, 0, 0, 0)

GROUND: Rgba = (74, 108, 66, 255)
GROUND_RIM: Rgba = (52, 78, 48, 255)
ROAD: Rgba = (176, 150, 96, 255)
ROAD_KERB: Rgba = (132, 110, 68, 255)
SHADOW: Rgba = (0, 0, 0, 80)
OUTLINE: Rgba = (24, 24, 32, 255)

# A walker's colour says which way it is facing.
# A turn is then visible in a still frame, not only in motion.
FACING_COLORS: tuple[Rgba, ...] = (
    (232, 96, 96, 255),
    (232, 200, 96, 255),
    (96, 200, 232, 255),
    (168, 120, 232, 255),
)

# Which way each facing points on screen, in half-tiles.
# North is -y in grid space, which the projection shears up and right.
FACING_STEPS: tuple[tuple[int, int], ...] = (
    (1, -1),
    (1, 1),
    (-1, 1),
    (-1, -1),
)


def grid_coords(px: int, py: int) -> tuple[float, float]:
    """Get a pixel's position within its cell, in grid units."""
    dx = (px + 0.5) - TILE_WIDTH / 2
    dy = (py + 0.5) - TILE_HEIGHT / 2

    east = (dx / (TILE_WIDTH / 2) + dy / (TILE_HEIGHT / 2)) / 2
    south = (dy / (TILE_HEIGHT / 2) - dx / (TILE_WIDTH / 2)) / 2

    return east, south


def shade(color: Rgba, amount: int) -> Rgba:
    red, green, blue, alpha = color
    return (
        max(0, min(255, red + amount)),
        max(0, min(255, green + amount)),
        max(0, min(255, blue + amount)),
        alpha,
    )


def noise(px: int, py: int, spread: int) -> int:
    """Get a deterministic per-pixel shade offset."""
    mixed = (px * 73856093) ^ (py * 19349663)
    return (mixed & 0xFF) % (2 * spread + 1) - spread


def inside_cell(east: float, south: float) -> bool:
    return abs(east) <= 0.5 and abs(south) <= 0.5


def on_rim(east: float, south: float) -> bool:
    return max(abs(east), abs(south)) > 0.5 - RIM


def ground_pixel(px: int, py: int) -> Rgba:
    east, south = grid_coords(px, py)

    if not inside_cell(east, south):
        return TRANSPARENT

    # The rim is what draws the lattice.
    # Painting it into the tile is why the scene draws no lines of its own.
    if on_rim(east, south):
        return shade(GROUND_RIM, noise(px, py, 4))

    return shade(GROUND, noise(px, py, 7))


def on_road(east: float, south: float, links: int, half: float) -> bool:
    # The junction itself is paved whatever the links are.
    # An isolated tile is then a patch rather than nothing.
    if max(abs(east), abs(south)) <= half:
        return True

    if links & LINK_NORTH and south <= 0 and abs(east) <= half:
        return True

    if links & LINK_EAST and east >= 0 and abs(south) <= half:
        return True

    if links & LINK_SOUTH and south >= 0 and abs(east) <= half:
        return True

    if links & LINK_WEST and east <= 0 and abs(south) <= half:
        return True

    return False


def road_pixel(px: int, py: int, links: int) -> Rgba:
    east, south = grid_coords(px, py)

    if not inside_cell(east, south):
        return TRANSPARENT

    if on_road(east, south, links, ROAD_HALF):
        return shade(ROAD, noise(px, py, 6))

    if on_road(east, south, links, ROAD_HALF + RIM):
        return shade(ROAD_KERB, noise(px, py, 4))

    return ground_pixel(px, py)


def in_ellipse(
    px: int, py: int, cx: float, cy: float, rx: float, ry: float
) -> bool:
    dx = (px + 0.5 - cx) / rx
    dy = (py + 0.5 - cy) / ry

    return dx * dx + dy * dy <= 1.0


def walker_pixel(px: int, py: int, facing: int) -> Rgba:
    east, south = grid_coords(px, py)

    if not inside_cell(east, south):
        return TRANSPARENT

    body = FACING_COLORS[facing]
    step_x, step_y = FACING_STEPS[facing]
    nose_x = TILE_WIDTH / 2 + step_x * 11.0
    nose_y = TILE_HEIGHT / 2 - 4.0 + step_y * 5.5

    if in_ellipse(px, py, nose_x, nose_y, 5.0, 5.0):
        return shade(body, 48)

    if in_ellipse(px, py, TILE_WIDTH / 2, TILE_HEIGHT / 2 - 4.0, 13.0, 13.0):
        if in_ellipse(
            px, py, TILE_WIDTH / 2, TILE_HEIGHT / 2 - 4.0, 11.0, 11.0
        ):
            return body

        return OUTLINE

    # A shadow on the ground is what stops the figure looking pasted on.
    if in_ellipse(px, py, TILE_WIDTH / 2, TILE_HEIGHT / 2 + 11.0, 17.0, 8.5):
        return SHADOW

    return TRANSPARENT


def slot_origin(slot: int) -> tuple[int, int]:
    return (slot % COLUMNS) * TILE_WIDTH, (slot // COLUMNS) * TILE_HEIGHT


def build_atlas() -> tuple[int, int, bytearray]:
    """Draw every slot into one straight-RGBA image."""
    width = COLUMNS * TILE_WIDTH
    height = ROWS * TILE_HEIGHT
    pixels = bytearray(width * height * 4)

    painters = {GROUND_SLOT: ground_pixel}

    for links in range(ROAD_SLOT_COUNT):
        painters[FIRST_ROAD_SLOT + links] = (
            lambda px, py, links=links: road_pixel(px, py, links)
        )

    for facing in range(WALKER_SLOT_COUNT):
        painters[FIRST_WALKER_SLOT + facing] = (
            lambda px, py, facing=facing: walker_pixel(px, py, facing)
        )

    for slot, painter in painters.items():
        left, top = slot_origin(slot)

        for py in range(TILE_HEIGHT):
            for px in range(TILE_WIDTH):
                at = ((top + py) * width + left + px) * 4
                pixels[at : at + 4] = bytes(painter(px, py))

    return width, height, pixels


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + kind
        + payload
        + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)
    )


def png_bytes(width: int, height: int, pixels: bytes) -> bytes:
    """Encode straight RGBA as an 8-bit PNG, one filter byte per row."""
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


def render() -> bytes:
    width, height, pixels = build_atlas()
    return png_bytes(width, height, bytes(pixels))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=DEFAULT_ROOT,
        help="Repository root (defaults to the parent of scripts/)",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Only report whether the committed atlas is up to date",
    )
    args = parser.parse_args()

    target = args.root / ATLAS_PATH
    wanted = render()

    if args.check:
        if not target.exists():
            print(f"Missing: {target}")
            return 1

        if target.read_bytes() != wanted:
            print(f"Stale: {target}")
            print("Regenerate it with scripts/generate_game_atlas.py.")
            return 1

        print(f"OK: {target} matches the generator")
        return 0

    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(wanted)

    print(f"OK: wrote {len(wanted)} bytes to {target}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
