#!/usr/bin/env python3

import argparse
import math
import re
import struct
import sys
import zlib
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent

ATLAS_PATH = Path("src/apps/poker/assets/atlas.png")

POKER_ATLAS_HEADER = Path(
    "src/apps/poker/include/antwika/poker/PokerAtlas.hpp"
)
CARD_HEADER = Path("src/libs/holdem/include/antwika/holdem/Card.hpp")

class LayoutError(Exception):
    pass


def read_header(relative: Path) -> str:
    path = DEFAULT_ROOT / relative
    try:
        return path.read_text(encoding="utf-8")
    except OSError as error:
        raise LayoutError(f"Cannot read {path}: {error}") from error

def integer_constant(source: str, name: str, where: Path) -> int:
    match = re.search(
        rf"constexpr\s+[\w:]+\s+{name}\s*=\s*(\d+)\s*;", source
    )

    if match is None:
        raise LayoutError(f"{where}: no integer constant named {name}")

    return int(match.group(1))

def size_constant(source: str, name: str, where: Path) -> tuple[int, int]:
    match = re.search(
        rf"constexpr\s+Size\s+{name}\s*\{{\s*"
        rf"\.width\s*=\s*(\d+)\s*,\s*\.height\s*=\s*(\d+)\s*\}}",
        source,
    )

    if match is None:
        raise LayoutError(f"{where}: no Size constant named {name}")

    return int(match.group(1)), int(match.group(2))

ATLAS_SOURCE = read_header(POKER_ATLAS_HEADER)
CARD_SOURCE = read_header(CARD_HEADER)

SLOT_WIDTH, SLOT_HEIGHT = size_constant(
    ATLAS_SOURCE, "kAtlasSlotSize", POKER_ATLAS_HEADER
)
COLUMNS = integer_constant(ATLAS_SOURCE, "kAtlasColumns", POKER_ATLAS_HEADER)
ROWS = integer_constant(ATLAS_SOURCE, "kAtlasRows", POKER_ATLAS_HEADER)

CARD_FACE_SLOT = integer_constant(
    ATLAS_SOURCE, "kCardFaceSlot", POKER_ATLAS_HEADER
)
CARD_BACK_SLOT = integer_constant(
    ATLAS_SOURCE, "kCardBackSlot", POKER_ATLAS_HEADER
)
FELT_SLOT = integer_constant(ATLAS_SOURCE, "kFeltSlot", POKER_ATLAS_HEADER)
PLATE_SLOT = integer_constant(ATLAS_SOURCE, "kPlateSlot", POKER_ATLAS_HEADER)
CHAIR_SLOT = integer_constant(ATLAS_SOURCE, "kChairSlot", POKER_ATLAS_HEADER)
CHIP_SLOT = integer_constant(ATLAS_SOURCE, "kChipSlot", POKER_ATLAS_HEADER)
DEALER_SLOT = integer_constant(
    ATLAS_SOURCE, "kDealerButtonSlot", POKER_ATLAS_HEADER
)
TO_ACT_SLOT = integer_constant(
    ATLAS_SOURCE, "kToActSlot", POKER_ATLAS_HEADER
)
TABLE_SLOT = integer_constant(
    ATLAS_SOURCE, "kTableSlot", POKER_ATLAS_HEADER
)
FIRST_SUIT_SLOT = integer_constant(
    ATLAS_SOURCE, "kFirstSuitSlot", POKER_ATLAS_HEADER
)
FIRST_RANK_SLOT = integer_constant(
    ATLAS_SOURCE, "kFirstRankSlot", POKER_ATLAS_HEADER
)

SUIT_COUNT = integer_constant(CARD_SOURCE, "kSuitCount", CARD_HEADER)
RANK_COUNT = integer_constant(CARD_SOURCE, "kRankCount", CARD_HEADER)

Rgba = tuple[int, int, int, int]

TRANSPARENT: Rgba = (0, 0, 0, 0)
WHITE: Rgba = (255, 255, 255, 255)
PAPER: Rgba = (243, 241, 234, 255)
PAPER_EDGE: Rgba = (176, 172, 160, 255)
BACK: Rgba = (28, 52, 116, 255)
BACK_LINE: Rgba = (72, 104, 184, 255)
FELT: Rgba = (14, 78, 50, 255)
PLATE: Rgba = (22, 28, 34, 235)
PLATE_EDGE: Rgba = (86, 96, 104, 255)
CHAIR: Rgba = (52, 34, 24, 255)
TABLE_RAIL: Rgba = (58, 38, 26, 255)
TABLE_FELT: Rgba = (16, 84, 54, 255)
CHIP_BODY: Rgba = (196, 48, 48, 255)
CHIP_EDGE: Rgba = (240, 240, 240, 255)
BUTTON: Rgba = (246, 246, 240, 255)
BUTTON_EDGE: Rgba = (40, 40, 44, 255)
MARKER: Rgba = (240, 200, 72, 255)

RANK_GLYPHS = (
    ("111", "001", "111", "100", "111"),
    ("111", "001", "111", "001", "111"),
    ("101", "101", "111", "001", "001"),
    ("111", "100", "111", "001", "111"),
    ("111", "100", "111", "101", "111"),
    ("111", "001", "001", "001", "001"),
    ("111", "101", "111", "101", "111"),
    ("111", "101", "111", "001", "111"),
    ("111", "010", "010", "010", "010"),
    ("001", "001", "001", "101", "111"),
    ("111", "101", "101", "111", "011"),
    ("101", "101", "110", "101", "101"),
    ("111", "101", "111", "101", "101"),
)

def check_layout() -> None:
    if len(RANK_GLYPHS) != RANK_COUNT:
        raise LayoutError(
            f"{RANK_COUNT} ranks declared, {len(RANK_GLYPHS)} drawn"
        )

    if FIRST_SUIT_SLOT + SUIT_COUNT > FIRST_RANK_SLOT:
        raise LayoutError("the suit slots run into the rank slots")

    if FIRST_RANK_SLOT + RANK_COUNT > COLUMNS * ROWS:
        raise LayoutError("the rank slots run off the end of the atlas")

def unit(value: int, extent: int) -> float:
    return (value + 0.5) / extent

def rounded(east: float, south: float, inset: float, radius: float) -> bool:
    left = inset
    right = 1.0 - inset
    if east < left or east > right or south < left or south > right:
        return False

    near_x = min(east - left, right - east)
    near_y = min(south - left, right - south)
    if near_x >= radius or near_y >= radius:
        return True

    away_x = radius - near_x
    away_y = radius - near_y

    return away_x * away_x + away_y * away_y <= radius * radius

def disc(east: float, south: float, radius: float) -> bool:
    dx = east - 0.5
    dy = south - 0.5

    return dx * dx + dy * dy <= radius * radius

def noise(px: int, py: int, spread: int) -> int:
    mixed = (px * 73_856_093) ^ (py * 19_349_663)

    return (mixed % (2 * spread + 1)) - spread

def shade(color: Rgba, amount: int) -> Rgba:
    red, green, blue, alpha = color

    return (
        max(0, min(255, red + amount)),
        max(0, min(255, green + amount)),
        max(0, min(255, blue + amount)),
        alpha,
    )

def card_face_pixel(px: int, py: int) -> Rgba:
    east = unit(px, SLOT_WIDTH)
    south = unit(py, SLOT_HEIGHT)

    if not rounded(east, south, 0.03, 0.12):
        return TRANSPARENT

    if not rounded(east, south, 0.09, 0.10):
        return PAPER_EDGE

    return PAPER

def card_back_pixel(px: int, py: int) -> Rgba:
    east = unit(px, SLOT_WIDTH)
    south = unit(py, SLOT_HEIGHT)

    if not rounded(east, south, 0.03, 0.12):
        return TRANSPARENT

    if not rounded(east, south, 0.09, 0.10):
        return PAPER

    if (px + py) % 6 == 0 or (px - py) % 6 == 0:
        return BACK_LINE

    return BACK

def felt_pixel(px: int, py: int) -> Rgba:
    return shade(FELT, noise(px, py, 5))

def plate_pixel(px: int, py: int) -> Rgba:
    east = unit(px, SLOT_WIDTH)
    south = unit(py, SLOT_HEIGHT)

    if not rounded(east, south, 0.02, 0.16):
        return TRANSPARENT

    if not rounded(east, south, 0.08, 0.14):
        return PLATE_EDGE

    return PLATE

def table_pixel(px: int, py: int) -> Rgba:
    east = unit(px, SLOT_WIDTH)
    south = unit(py, SLOT_HEIGHT)

    if not rounded(east, south, 0.01, 0.34):
        return TRANSPARENT

    if not rounded(east, south, 0.09, 0.30):
        return TABLE_RAIL

    return shade(TABLE_FELT, noise(px, py, 4))

def chair_pixel(px: int, py: int) -> Rgba:
    east = unit(px, SLOT_WIDTH)
    south = unit(py, SLOT_HEIGHT)

    if rounded(east, south, 0.14, 0.30) and south < 0.55:
        return shade(CHAIR, 22)

    if rounded(east, south, 0.06, 0.20) and south > 0.45:
        return CHAIR

    return TRANSPARENT

def chip_pixel(px: int, py: int) -> Rgba:
    east = unit(px, SLOT_WIDTH)
    south = unit(py, SLOT_HEIGHT)

    if not disc(east, south, 0.42):
        return TRANSPARENT

    if not disc(east, south, 0.34):
        angle = math.atan2(south - 0.5, east - 0.5)
        if int((angle + math.pi) / (math.pi / 6)) % 2 == 0:
            return CHIP_EDGE

        return CHIP_BODY

    if disc(east, south, 0.16):
        return CHIP_EDGE

    return CHIP_BODY

def dealer_pixel(px: int, py: int) -> Rgba:
    east = unit(px, SLOT_WIDTH)
    south = unit(py, SLOT_HEIGHT)

    if not disc(east, south, 0.40):
        return TRANSPARENT

    if not disc(east, south, 0.32):
        return BUTTON_EDGE

    if 0.34 <= east <= 0.42 and 0.28 <= south <= 0.72:
        return BUTTON_EDGE

    if disc(east - 0.06, south, 0.22) and not disc(east - 0.06, south, 0.13):
        return BUTTON_EDGE

    return BUTTON

def to_act_pixel(px: int, py: int) -> Rgba:
    east = unit(px, SLOT_WIDTH)
    south = unit(py, SLOT_HEIGHT)

    if south < 0.15 or south > 0.85:
        return TRANSPARENT

    half = (0.85 - south) * 0.5

    if abs(east - 0.5) > half:
        return TRANSPARENT

    return MARKER

def suit_pixel(px: int, py: int, suit: int) -> Rgba:
    east = unit(px, SLOT_WIDTH)
    south = unit(py, SLOT_HEIGHT)

    if suit == 1:
        if abs(east - 0.5) + abs(south - 0.5) <= 0.40:
            return WHITE

        return TRANSPARENT

    if suit == 2:
        if disc(east + 0.16, south + 0.10, 0.22):
            return WHITE
        if disc(east - 0.16, south + 0.10, 0.22):
            return WHITE
        if south >= 0.5 and abs(east - 0.5) <= (0.92 - south) * 0.62:
            return WHITE

        return TRANSPARENT

    if suit == 3:
        if disc(east + 0.16, south - 0.14, 0.22):
            return WHITE
        if disc(east - 0.16, south - 0.14, 0.22):
            return WHITE
        if south <= 0.5 and abs(east - 0.5) <= (south - 0.08) * 0.62:
            return WHITE
        if south > 0.55 and abs(east - 0.5) <= (south - 0.5) * 0.55:
            return WHITE

        return TRANSPARENT

    if disc(east, south + 0.18, 0.20):
        return WHITE
    if disc(east + 0.20, south - 0.10, 0.20):
        return WHITE
    if disc(east - 0.20, south - 0.10, 0.20):
        return WHITE
    if south > 0.55 and abs(east - 0.5) <= (south - 0.5) * 0.55:
        return WHITE

    return TRANSPARENT

def rank_pixel(px: int, py: int, rank: int) -> Rgba:
    glyph = RANK_GLYPHS[rank]

    scale = min(SLOT_WIDTH // 4, SLOT_HEIGHT // 6)
    left = (SLOT_WIDTH - 3 * scale) // 2
    top = (SLOT_HEIGHT - 5 * scale) // 2

    column = (px - left) // scale
    row = (py - top) // scale

    if px < left or py < top or column >= 3 or row >= 5:
        return TRANSPARENT

    if glyph[row][column] == "0":
        return TRANSPARENT

    return WHITE

def check_slots(slots: list[int]) -> None:
    duplicated = sorted({s for s in slots if slots.count(s) > 1})
    if duplicated:
        raise LayoutError(f"two painters share slot(s) {duplicated}")

    capacity = COLUMNS * ROWS
    outside = sorted(s for s in slots if not 0 <= s < capacity)
    if outside:
        raise LayoutError(
            f"slot(s) {outside} are outside a {COLUMNS}x{ROWS} atlas"
        )

def slot_origin(slot: int) -> tuple[int, int]:
    return (slot % COLUMNS) * SLOT_WIDTH, (slot // COLUMNS) * SLOT_HEIGHT

def build_atlas() -> tuple[int, int, bytearray]:
    check_layout()

    width = COLUMNS * SLOT_WIDTH
    height = ROWS * SLOT_HEIGHT
    pixels = bytearray(width * height * 4)

    painters = [
        (CARD_FACE_SLOT, card_face_pixel),
        (CARD_BACK_SLOT, card_back_pixel),
        (FELT_SLOT, felt_pixel),
        (TABLE_SLOT, table_pixel),
        (PLATE_SLOT, plate_pixel),
        (CHAIR_SLOT, chair_pixel),
        (CHIP_SLOT, chip_pixel),
        (DEALER_SLOT, dealer_pixel),
        (TO_ACT_SLOT, to_act_pixel),
    ]

    for suit in range(SUIT_COUNT):
        painters.append(
            (
                FIRST_SUIT_SLOT + suit,
                lambda px, py, suit=suit: suit_pixel(px, py, suit),
            )
        )

    for rank in range(RANK_COUNT):
        painters.append(
            (
                FIRST_RANK_SLOT + rank,
                lambda px, py, rank=rank: rank_pixel(px, py, rank),
            )
        )

    check_slots([slot for slot, _ in painters])

    for slot, painter in painters:
        left, top = slot_origin(slot)

        for py in range(SLOT_HEIGHT):
            for px in range(SLOT_WIDTH):
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
    parser = argparse.ArgumentParser(
        description="Generate the poker table's atlas from PokerAtlas.hpp."
    )
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
            print("Regenerate it with scripts/generate_poker_atlas.py.")
            return 1

        print(f"OK: {target} matches the generator")
        return 0

    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(wanted)

    print(f"OK: wrote {len(wanted)} bytes to {target}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
