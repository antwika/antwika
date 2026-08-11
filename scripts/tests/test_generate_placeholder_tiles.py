#!/usr/bin/env python3

import importlib.util
import json
import struct
import sys
import tempfile
from pathlib import Path

SCRIPT_PATH = (
    Path(__file__).resolve().parent.parent
    / "generate_placeholder_tiles.py"
)
REPO_ROOT = Path(__file__).resolve().parent.parent.parent

spec = importlib.util.spec_from_file_location(
    "generate_placeholder_tiles", SCRIPT_PATH
)
generate_placeholder_tiles = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generate_placeholder_tiles)


def run_main(*arguments: str) -> int:
    old_argv = sys.argv
    sys.argv = ["generate_placeholder_tiles.py", *arguments]
    try:
        return generate_placeholder_tiles.main()
    finally:
        sys.argv = old_argv


def png_size(data: bytes) -> tuple[int, int]:
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    return struct.unpack(">II", data[16:24])


def outputs_by_name() -> dict[str, bytes]:
    return {
        str(relative).replace("\\", "/"): data
        for relative, data in
        generate_placeholder_tiles.render_outputs().items()
    }


def documents() -> dict[str, dict]:
    return {
        name: json.loads(data)
        for name, data in outputs_by_name().items()
        if name.endswith("tileset.json")
    }


def it_renders_the_same_bytes_every_time() -> None:
    assert (
        generate_placeholder_tiles.render_outputs()
        == generate_placeholder_tiles.render_outputs()
    )


def it_emits_a_tileset_per_terrain_and_the_shared_files() -> None:
    names = set(outputs_by_name())

    assert "assets/tilesets/system.png" in names
    assert "assets/tilesets/rules.json" in names

    for terrain in generate_placeholder_tiles.ALL_TERRAINS:
        assert f"assets/tilesets/default-{terrain}/tileset.json" in names
        assert f"assets/tilesets/default-{terrain}/layer-0.png" in names


def it_keeps_sprite_ids_unique_and_under_next_sprite_id() -> None:
    for document in documents().values():
        ids = [
            sprite["id"]
            for layer in document["layers"]
            for sprite in layer["sprites"]
        ]

        assert len(ids) == len(set(ids))
        assert max(ids) < document["nextSpriteId"]


def it_sizes_every_layer_image_to_its_sprite_count() -> None:
    outputs = outputs_by_name()

    for name, document in documents().items():
        directory = name.rsplit("/", 1)[0]

        for index, layer in enumerate(document["layers"]):
            width, height = png_size(
                outputs[f"{directory}/layer-{index}.png"]
            )

            assert width == generate_placeholder_tiles.ATLAS_WIDTH
            assert height == len(layer["sprites"]) * 8


def it_borders_every_terrain_with_edge_sockets() -> None:
    for document in documents().values():
        base = document["layers"][0]["sprites"]
        outward = [
            tuple(
                side
                for side, socket in sprite["sockets"].items()
                if socket == "edge"
            )
            for sprite in base
        ]

        for side in ("n", "e", "s", "w"):
            assert (side,) in outward

        for pair in (("n", "w"), ("n", "e"), ("s", "w"), ("s", "e")):
            assert tuple(sorted(pair)) in map(
                lambda sides: tuple(sorted(sides)), outward
            )


def it_restricts_the_floor_decor_to_listed_base_sprites() -> None:
    document = documents()["assets/tilesets/default-floor/tileset.json"]
    base_ids = {
        sprite["id"] for sprite in document["layers"][0]["sprites"]
    }
    decor = document["layers"][1]["sprites"]

    assert document["layers"][1]["density"] == 48
    assert len(decor) == 2

    for sprite in decor:
        assert sprite["on"]
        assert set(sprite["on"]) <= base_ids
        assert set(sprite["sockets"].values()) == {"open"}


def it_marks_only_the_pebble_with_a_non_default_weight() -> None:
    weighted = [
        (name, sprite)
        for name, document in documents().items()
        for layer in document["layers"]
        for sprite in layer["sprites"]
        if "weight" in sprite
    ]

    assert len(weighted) == 1

    name, pebble = weighted[0]

    assert name == "assets/tilesets/default-floor/tileset.json"
    assert pebble["weight"] == 2


def it_animates_the_water_interior_over_three_frames() -> None:
    document = documents()["assets/tilesets/default-water/tileset.json"]
    sprites = document["layers"][0]["sprites"]

    assert sprites[0]["frames"] == 3

    for sprite in sprites[1:]:
        assert "frames" not in sprite


def it_draws_the_system_sheet_as_four_pieces() -> None:
    assert png_size(generate_placeholder_tiles.system_png()) == (32, 8)


def it_writes_the_tilesets_and_then_reports_them_as_current() -> None:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        assert run_main("--root", str(root)) == 0
        assert (
            root / "assets/tilesets/default-wall/tileset.json"
        ).exists()
        assert run_main("--root", str(root), "--check") == 0


def it_reports_a_missing_tileset() -> None:
    with tempfile.TemporaryDirectory() as directory:
        assert run_main("--root", directory, "--check") == 1


def it_reports_a_stale_tileset() -> None:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        assert run_main("--root", str(root)) == 0
        target = root / "assets/tilesets/default-wall/tileset.json"
        target.write_text("{}\n", encoding="utf-8")
        assert run_main("--root", str(root), "--check") == 1


def it_keeps_the_committed_tilesets_in_step_with_the_generator() -> None:
    assert run_main("--root", str(REPO_ROOT), "--check") == 0


def main() -> None:
    tests = [
        it_renders_the_same_bytes_every_time,
        it_emits_a_tileset_per_terrain_and_the_shared_files,
        it_keeps_sprite_ids_unique_and_under_next_sprite_id,
        it_sizes_every_layer_image_to_its_sprite_count,
        it_borders_every_terrain_with_edge_sockets,
        it_restricts_the_floor_decor_to_listed_base_sprites,
        it_marks_only_the_pebble_with_a_non_default_weight,
        it_animates_the_water_interior_over_three_frames,
        it_draws_the_system_sheet_as_four_pieces,
        it_writes_the_tilesets_and_then_reports_them_as_current,
        it_reports_a_missing_tileset,
        it_reports_a_stale_tileset,
        it_keeps_the_committed_tilesets_in_step_with_the_generator,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
