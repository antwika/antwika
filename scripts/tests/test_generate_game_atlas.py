#!/usr/bin/env python3
# Plain-assert tests for generate_game_atlas.py.
# Run directly:
#   python3 scripts/tests/test_generate_game_atlas.py
import importlib.util
import sys
import tempfile
from pathlib import Path

SCRIPT_PATH = (
    Path(__file__).resolve().parent.parent / "generate_game_atlas.py"
)
REPO_ROOT = Path(__file__).resolve().parent.parent.parent

spec = importlib.util.spec_from_file_location(
    "generate_game_atlas", SCRIPT_PATH
)
generate_game_atlas = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generate_game_atlas)


def run_main(*arguments):
    old_argv = sys.argv
    sys.argv = ["generate_game_atlas.py", *arguments]
    try:
        return generate_game_atlas.main()
    finally:
        sys.argv = old_argv


def pixel_at(pixels, width, px, py):
    at = (py * width + px) * 4
    return tuple(pixels[at : at + 4])


def it_draws_every_slot_the_game_addresses():
    width, height, _ = generate_game_atlas.build_atlas()

    slots = (
        1
        + generate_game_atlas.ROAD_SLOT_COUNT
        + generate_game_atlas.WALKER_SLOT_COUNT
    )

    assert width == 8 * 128
    assert height == 3 * 64
    assert slots <= generate_game_atlas.COLUMNS * generate_game_atlas.ROWS


def it_leaves_the_corners_of_a_tile_transparent():
    width, _, pixels = generate_game_atlas.build_atlas()

    # A tile's cell is the bounding box of a diamond.
    # So its corners are outside the tile and its middle is inside.
    assert pixel_at(pixels, width, 0, 0)[3] == 0
    assert pixel_at(pixels, width, 127, 63)[3] == 0
    assert pixel_at(pixels, width, 64, 32)[3] == 255


def it_paves_a_road_towards_every_link_it_has():
    width, _, pixels = generate_game_atlas.build_atlas()

    slot = generate_game_atlas.FIRST_ROAD_SLOT + generate_game_atlas.LINK_EAST
    left, top = generate_game_atlas.slot_origin(slot)

    # East runs down and right from the middle of the tile.
    # West is unlinked, so up and left is still ground.
    paved = pixel_at(pixels, width, left + 80, top + 40)
    grass = pixel_at(pixels, width, left + 48, top + 24)

    assert paved[0] > 120
    assert grass[0] < 120


def it_gives_each_facing_a_colour_of_its_own():
    width, _, pixels = generate_game_atlas.build_atlas()

    seen = set()
    for facing in range(generate_game_atlas.WALKER_SLOT_COUNT):
        slot = generate_game_atlas.FIRST_WALKER_SLOT + facing
        left, top = generate_game_atlas.slot_origin(slot)
        seen.add(pixel_at(pixels, width, left + 64, top + 28))

    assert len(seen) == generate_game_atlas.WALKER_SLOT_COUNT


def it_renders_the_same_bytes_every_time():
    assert generate_game_atlas.render() == generate_game_atlas.render()


def it_writes_a_png_and_then_reports_it_as_current():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)

        assert run_main("--root", str(root), "--check") == 1
        assert run_main("--root", str(root)) == 0

        written = (root / generate_game_atlas.ATLAS_PATH).read_bytes()
        assert written.startswith(b"\x89PNG\r\n\x1a\n")

        assert run_main("--root", str(root), "--check") == 0


def it_reports_a_stale_atlas():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        target = root / generate_game_atlas.ATLAS_PATH
        target.parent.mkdir(parents=True)
        target.write_bytes(b"not the atlas")

        assert run_main("--root", str(root), "--check") == 1


# The committed picture is what the game actually draws from.
def it_keeps_the_committed_atlas_in_step_with_the_generator():
    assert run_main("--root", str(REPO_ROOT), "--check") == 0


def main():
    tests = [
        it_draws_every_slot_the_game_addresses,
        it_leaves_the_corners_of_a_tile_transparent,
        it_paves_a_road_towards_every_link_it_has,
        it_gives_each_facing_a_colour_of_its_own,
        it_renders_the_same_bytes_every_time,
        it_writes_a_png_and_then_reports_it_as_current,
        it_reports_a_stale_atlas,
        it_keeps_the_committed_atlas_in_step_with_the_generator,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
