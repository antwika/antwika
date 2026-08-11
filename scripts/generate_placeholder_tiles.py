#!/usr/bin/env python3

import argparse
import json
import struct
import sys
import zlib
from collections.abc import Callable
from pathlib import Path

Pattern = Callable[[int, int], bool]

DEFAULT_ROOT = Path(__file__).resolve().parent.parent

TILESETS_DIR = Path("assets/tilesets")

SPRITE = 8

MAX_FRAMES = 4

ATLAS_WIDTH = MAX_FRAMES * SPRITE

DEFAULT_DENSITY = 64

DEFAULT_WEIGHT = 4

INK = (255, 255, 255, 255)
PAPER = (128, 128, 128, 255)
TRANSPARENT = (0, 0, 0, 0)

EDGE_SIDES = {
    "edge_n": ("n",),
    "edge_e": ("e",),
    "edge_s": ("s",),
    "edge_w": ("w",),
    "corner_nw": ("n", "w"),
    "corner_ne": ("n", "e"),
    "corner_sw": ("s", "w"),
    "corner_se": ("s", "e"),
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


def dirt_ink(x: int, y: int) -> bool:
    return (x % 4 == 1 and y % 4 == 3) or (x % 4 == 3 and y % 4 == 1)


def water_phase_ink(phase: int, x: int, y: int) -> bool:
    return y % 4 == 2 and (x + phase) % 4 != 3


def band_ink(x: int, y: int) -> bool:
    return x % 2 == 0 or y % 4 == 0


def rim_ink(x: int, y: int) -> bool:
    return y < 2 or x % 4 == 0


def bridge_ink(x: int, y: int) -> bool:
    return y % 4 != 0 and (x + 2 * (y // 4)) % 8 != 7


def shade_ink(x: int, y: int) -> bool:
    return (x + y) % 2 == 0


FLOWER_MOTIF = frozenset(
    ((3, 2), (2, 3), (3, 3), (4, 3), (3, 4), (3, 5), (3, 6))
)

PEBBLE_MOTIF = frozenset(
    ((4, 5), (5, 5), (3, 6), (4, 6), (5, 6), (6, 6))
)


def base_frame(pattern: Pattern) -> list[str]:
    return [
        "ink" if pattern(x, y) else "paper"
        for y in range(SPRITE)
        for x in range(SPRITE)
    ]


def bordered_frame(
    pattern: Pattern, sides: tuple[str, ...]
) -> list[str]:
    def rimmed(x: int, y: int) -> bool:
        if "n" in sides and (y == 0 or (y == 1 and x % 2 == 0)):
            return True

        if "s" in sides and (y == 7 or (y == 6 and x % 2 == 0)):
            return True

        if "w" in sides and (x == 0 or (x == 1 and y % 2 == 0)):
            return True

        return "e" in sides and (x == 7 or (x == 6 and y % 2 == 0))

    return base_frame(lambda x, y: rimmed(x, y) or pattern(x, y))


def decor_frame(motif: frozenset) -> list[str]:
    return [
        "ink" if (x, y) in motif else None
        for y in range(SPRITE)
        for x in range(SPRITE)
    ]


def sprite(
    sockets: dict,
    frames: list,
    on: tuple = (),
    weight: int = DEFAULT_WEIGHT,
) -> dict:
    return {
        "sockets": sockets,
        "frames": frames,
        "on": list(on),
        "weight": weight,
    }


def same_socket(name: str) -> dict:
    return {"n": name, "e": name, "s": name, "w": name}


def edge_sockets(interior: str, sides: tuple[str, ...]) -> dict:
    return {
        side: "edge" if side in sides else interior
        for side in ("n", "e", "s", "w")
    }


def border_sprites(interior: str, pattern: Pattern) -> list[dict]:
    return [
        sprite(
            edge_sockets(interior, sides),
            [bordered_frame(pattern, sides)],
        )
        for sides in EDGE_SIDES.values()
    ]


def bordered_tileset(
    terrain: str, socket: str, pattern: Pattern
) -> dict:
    sprites = [sprite(same_socket(socket), [base_frame(pattern)])]
    sprites += border_sprites(socket, pattern)

    return {
        "name": f"default-{terrain}",
        "terrain": terrain,
        "layers": [{"name": "base", "sprites": sprites}],
    }


def floor_tileset() -> dict:
    def grass_a(x: int, y: int) -> bool:
        return pattern_ink("floor", x, y)

    def grass_b(x: int, y: int) -> bool:
        return pattern_ink("floor", (x + 2) % 8, y)

    base = [
        sprite(same_socket("grass"), [base_frame(grass_a)]),
        sprite(same_socket("grass"), [base_frame(grass_b)]),
        sprite(same_socket("dirt"), [base_frame(dirt_ink)]),
    ]
    base += border_sprites("grass", grass_a)

    decor = [
        sprite(same_socket("open"), [decor_frame(FLOWER_MOTIF)], (0,)),
        sprite(
            same_socket("open"),
            [decor_frame(PEBBLE_MOTIF)],
            (0, 1),
            weight=2,
        ),
    ]

    return {
        "name": "default-floor",
        "terrain": "floor",
        "layers": [
            {"name": "base", "sprites": base},
            {"name": "decor", "density": 48, "sprites": decor},
        ],
    }


def water_tileset() -> dict:
    ripple = [
        base_frame(lambda x, y, p=phase: water_phase_ink(p, x, y))
        for phase in range(3)
    ]

    def calm(x: int, y: int) -> bool:
        return water_phase_ink(0, x, y)

    sprites = [sprite(same_socket("water"), ripple)]
    sprites += border_sprites("water", calm)

    return {
        "name": "default-water",
        "terrain": "water",
        "layers": [{"name": "base", "sprites": sprites}],
    }


def tilesets() -> list[dict]:
    def plain(terrain: str) -> dict:
        return bordered_tileset(
            terrain,
            terrain,
            lambda x, y: pattern_ink(terrain, x, y),
        )

    return [
        floor_tileset(),
        plain("wall"),
        water_tileset(),
        plain("cliff"),
        plain("path"),
        plain("stair"),
    ]


def tileset_json_text(tileset: dict) -> str:
    next_id = 0
    layers = []

    for index, layer in enumerate(tileset["layers"]):
        sprites = []

        for entry in layer["sprites"]:
            record = {"id": next_id}
            next_id += 1

            if len(entry["frames"]) != 1:
                record["frames"] = len(entry["frames"])

            if entry["weight"] != DEFAULT_WEIGHT:
                record["weight"] = entry["weight"]

            record["sockets"] = entry["sockets"]
            record["on"] = entry["on"]
            sprites.append(record)

        record = {"name": layer["name"]}
        density = layer.get("density", DEFAULT_DENSITY)

        if index >= 1 and density != DEFAULT_DENSITY:
            record["density"] = density

        record["sprites"] = sprites
        layers.append(record)

    document = {
        "schema": 1,
        "name": tileset["name"],
        "terrain": tileset["terrain"],
        "nextSpriteId": next_id,
        "layers": layers,
    }

    return json.dumps(document, indent=2) + "\n"


def layer_png(sprites: list[dict]) -> bytes:
    height = len(sprites) * SPRITE
    pixels = bytearray(ATLAS_WIDTH * height * 4)

    for row, entry in enumerate(sprites):
        for slot, frame in enumerate(entry["frames"]):
            for y in range(SPRITE):
                for x in range(SPRITE):
                    kind = frame[y * SPRITE + x]

                    if kind is None:
                        continue

                    color = INK if kind == "ink" else PAPER
                    at = (
                        (row * SPRITE + y) * ATLAS_WIDTH
                        + slot * SPRITE
                        + x
                    ) * 4
                    pixels[at : at + 4] = bytes(color)

    return png_bytes(ATLAS_WIDTH, height, bytes(pixels))


def system_png() -> bytes:
    pieces = (band_ink, rim_ink, bridge_ink, shade_ink)
    width = len(pieces) * SPRITE
    pixels = bytearray(width * SPRITE * 4)

    for slot, piece in enumerate(pieces):
        for y in range(SPRITE):
            for x in range(SPRITE):
                if piece(x, y):
                    at = (y * width + slot * SPRITE + x) * 4
                    pixels[at : at + 4] = bytes(INK)

    return png_bytes(width, SPRITE, bytes(pixels))


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


FORBIDDEN = (
    ("wall", "water"),
    ("wall", "cliff"),
    ("water", "path"),
    ("water", "cliff"),
    ("water", "stair"),
    ("path", "cliff"),
    ("cliff", "stair"),
)

ALL_TERRAINS = ("floor", "wall", "water", "cliff", "path", "stair")

WEIGHTS = {"floor": 8, "wall": 3, "water": 2, "cliff": 1, "path": 2}


def rules_text() -> str:
    allowed = []

    for a_index, a_name in enumerate(ALL_TERRAINS):
        for b_name in ALL_TERRAINS[a_index:]:
            pair = tuple(sorted((a_name, b_name)))

            if pair in (tuple(sorted(f)) for f in FORBIDDEN):
                continue

            allowed.append([a_name, b_name])

    document = {"weights": WEIGHTS, "adjacency": allowed}

    return json.dumps(document, indent=2) + "\n"


def render_outputs() -> dict[Path, bytes]:
    outputs = {
        TILESETS_DIR / "system.png": system_png(),
        TILESETS_DIR / "rules.json": rules_text().encode("utf-8"),
    }

    for tileset in tilesets():
        directory = TILESETS_DIR / tileset["name"]
        outputs[directory / "tileset.json"] = tileset_json_text(
            tileset
        ).encode("utf-8")

        for index, layer in enumerate(tileset["layers"]):
            if layer["sprites"]:
                outputs[directory / f"layer-{index}.png"] = layer_png(
                    layer["sprites"]
                )

    return outputs


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate the placeholder tilesets."
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
        help="Only report whether the committed tilesets are up to date",
    )
    args = parser.parse_args()

    outputs = render_outputs()

    if args.check:
        failed = False

        for relative, wanted in sorted(outputs.items()):
            target = args.root / relative

            if not target.exists():
                print(f"Missing: {target}")
                failed = True
            elif target.read_bytes() != wanted:
                print(f"Stale: {target}")
                failed = True

        if failed:
            print(
                "Regenerate with scripts/generate_placeholder_tiles.py."
            )
            return 1

        print(f"OK: {len(outputs)} files match the generator")
        return 0

    for relative, wanted in sorted(outputs.items()):
        target = args.root / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(wanted)
        print(f"OK: wrote {target}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
