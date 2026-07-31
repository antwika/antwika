#!/usr/bin/env python3
# Draws the texture atlas apps/game renders its grid from.
# The art is generated rather than painted.
# So a tile's shape comes from the projection the game draws it with.
# The slot layout is read out of antwika/game/TileAtlas.hpp.
# Restating it here would be a second set of numbers to keep in step.
import argparse
import re
import struct
import sys
import zlib
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent

ATLAS_PATH = Path("src/apps/game/assets/atlas.png")

# The layout comes from these headers, the ones beside this script.
# --root says where the picture goes, not what it is drawn from.
TILE_ATLAS_HEADER = Path(
    "src/apps/game/include/antwika/game/TileAtlas.hpp"
)
DIRECTION_HEADER = Path(
    "src/apps/game/include/antwika/game/Direction.hpp"
)


class LayoutError(Exception):
    """Raised when a header does not say what the layout needs."""


def read_header(relative: Path) -> str:
    path = DEFAULT_ROOT / relative
    try:
        return path.read_text(encoding="utf-8")
    except OSError as error:
        raise LayoutError(f"Cannot read {path}: {error}") from error


def integer_constant(source: str, name: str, where: Path) -> int:
    """Get the value of an integer constexpr declared in a header."""
    # Only a literal, never an expression.
    # A constant the header derives is derived here the same way.
    match = re.search(
        rf"constexpr\s+[\w:]+\s+{name}\s*=\s*(\d+)\s*;", source
    )

    if match is None:
        raise LayoutError(f"{where}: no integer constant named {name}")

    return int(match.group(1))


def size_constant(source: str, name: str, where: Path) -> tuple[int, int]:
    """Get the width and height of a designated-initialiser Size."""
    match = re.search(
        rf"constexpr\s+Size\s+{name}\s*\{{\s*"
        rf"\.width\s*=\s*(\d+)\s*,\s*\.height\s*=\s*(\d+)\s*\}}",
        source,
    )

    if match is None:
        raise LayoutError(f"{where}: no Size constant named {name}")

    return int(match.group(1)), int(match.group(2))


def enumerators(source: str, name: str, where: Path) -> tuple[str, ...]:
    """Get a scoped enum's enumerator names, in declaration order."""
    match = re.search(
        rf"enum\s+class\s+{name}\s*(?::[^{{]*)?\{{([^}}]*)\}}", source
    )

    if match is None:
        raise LayoutError(f"{where}: no scoped enum named {name}")

    body = re.sub(r"//[^\n]*", "", match.group(1))
    found = tuple(
        entry.split("=")[0].strip()
        for entry in body.split(",")
        if entry.split("=")[0].strip()
    )

    if not found:
        raise LayoutError(f"{where}: enum {name} has no enumerators")

    return found


ATLAS_SOURCE = read_header(TILE_ATLAS_HEADER)
DIRECTION_SOURCE = read_header(DIRECTION_HEADER)

TILE_WIDTH, TILE_HEIGHT = size_constant(
    ATLAS_SOURCE, "kAtlasTileSize", TILE_ATLAS_HEADER
)
COLUMNS = integer_constant(ATLAS_SOURCE, "kAtlasColumns", TILE_ATLAS_HEADER)
ROWS = integer_constant(ATLAS_SOURCE, "kAtlasRows", TILE_ATLAS_HEADER)

GROUND_SLOT = integer_constant(
    ATLAS_SOURCE, "kGroundSlot", TILE_ATLAS_HEADER
)
FIRST_ROAD_SLOT = integer_constant(
    ATLAS_SOURCE, "kFirstRoadSlot", TILE_ATLAS_HEADER
)
ROAD_SLOT_COUNT = integer_constant(
    ATLAS_SOURCE, "kRoadSlotCount", TILE_ATLAS_HEADER
)
BUILDING_SLOT_COUNT = integer_constant(
    ATLAS_SOURCE, "kBuildingSlotCount", TILE_ATLAS_HEADER
)

# The directions the game has, in the order linkBit() numbers them.
DIRECTIONS = enumerators(DIRECTION_SOURCE, "Direction", DIRECTION_HEADER)

# kFirstWalkerSlot is derived in the header, so it is derived here.
FIRST_WALKER_SLOT = FIRST_ROAD_SLOT + ROAD_SLOT_COUNT
WALKER_SLOT_COUNT = len(DIRECTIONS)

# kFirstBuildingSlot is derived the same way, for the same reason.
FIRST_BUILDING_SLOT = FIRST_WALKER_SLOT + WALKER_SLOT_COUNT

# linkBit() is 1 << directionIndex(), so a bit is a place in the enum.
# Reordering Direction therefore moves the art with it.
LINK_BITS = {
    name: 1 << index for index, name in enumerate(DIRECTIONS)
}


def link_bit(name: str) -> int:
    if name not in LINK_BITS:
        raise LayoutError(
            f"{DIRECTION_HEADER}: Direction has no enumerator {name}"
        )

    return LINK_BITS[name]


LINK_NORTH = link_bit("North")
LINK_EAST = link_bit("East")
LINK_SOUTH = link_bit("South")
LINK_WEST = link_bit("West")

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

# One building per tool that places one, in BuildTool's own order.
# Each is a footprint half-width, a wall height, a roof and a wall.
# The footprint and the wall keep a building inside its cell.
# The roof is the footprint raised by the wall.
# So a tall building needs a small footprint.
# Otherwise its ridge leaves the diamond and gets clipped.
BUILDINGS: tuple[tuple[float, float, Rgba, Rgba], ...] = (
    (0.34, 8.0, (198, 104, 84, 255), (206, 190, 162, 255)),
    (0.38, 10.0, (86, 146, 202, 255), (228, 218, 196, 255)),
    (0.24, 14.0, (146, 148, 168, 255), (182, 186, 200, 255)),
)


def check_layout() -> None:
    """Fail loudly when the header asks for art this cannot draw."""
    # A road tile per link mask, and a mask bit per direction.
    # So the two counts move together or the numbering is wrong.
    if ROAD_SLOT_COUNT != 1 << WALKER_SLOT_COUNT:
        raise LayoutError(
            f"kRoadSlotCount is {ROAD_SLOT_COUNT}, but "
            f"{WALKER_SLOT_COUNT} directions make "
            f"{1 << WALKER_SLOT_COUNT} link masks"
        )

    # The buildings are written out here rather than derived.
    # A fourth building tool would otherwise be left undrawable.
    if len(BUILDINGS) != BUILDING_SLOT_COUNT:
        raise LayoutError(
            f"{len(BUILDINGS)} buildings drawn for "
            f"{BUILDING_SLOT_COUNT} building slots"
        )

    slots = (
        1 + ROAD_SLOT_COUNT + WALKER_SLOT_COUNT + BUILDING_SLOT_COUNT
    )
    if slots > COLUMNS * ROWS:
        raise LayoutError(
            f"{slots} slots do not fit in {COLUMNS}x{ROWS} tiles"
        )

    # The two per-facing tables are written out rather than derived.
    # A fifth direction would silently leave its walker undrawable.
    if len(FACING_COLORS) != WALKER_SLOT_COUNT:
        raise LayoutError(
            f"{len(FACING_COLORS)} facing colours for "
            f"{WALKER_SLOT_COUNT} directions"
        )

    if len(FACING_STEPS) != WALKER_SLOT_COUNT:
        raise LayoutError(
            f"{len(FACING_STEPS)} facing steps for "
            f"{WALKER_SLOT_COUNT} directions"
        )


check_layout()


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


def building_pixel(px: int, py: int, kind: int) -> Rgba:
    half, wall, roof, side = BUILDINGS[kind]

    # The roof is the footprint seen `wall` pixels further down.
    # So one footprint test, asked twice, draws a whole box.
    roof_east, roof_south = grid_coords(px, py + wall)
    if max(abs(roof_east), abs(roof_south)) <= half:
        return shade(roof, noise(px, py, 5))

    base_east, base_south = grid_coords(px, py)
    if max(abs(base_east), abs(base_south)) <= half:
        # Which wall is showing, from which side of the ridge it is on.
        # The left one is turned away from the light, so it is darker.
        lit = -18 if px + 0.5 < TILE_WIDTH / 2 else 10
        return shade(side, lit + noise(px, py, 4))

    # The same shadow a walker gets, and for the same reason.
    if in_ellipse(
        px, py, TILE_WIDTH / 2, TILE_HEIGHT / 2 + 4.0, 26.0, 13.0
    ):
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

    for kind in range(BUILDING_SLOT_COUNT):
        painters[FIRST_BUILDING_SLOT + kind] = (
            lambda px, py, kind=kind: building_pixel(px, py, kind)
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
