#!/usr/bin/env python3
# Plain-assert tests for generate_poker_atlas.py.
# Run directly:
#   python3 scripts/tests/test_generate_poker_atlas.py
import importlib.util
import sys
import tempfile
from pathlib import Path

SCRIPT_PATH = (
    Path(__file__).resolve().parent.parent / "generate_poker_atlas.py"
)
REPO_ROOT = Path(__file__).resolve().parent.parent.parent

spec = importlib.util.spec_from_file_location(
    "generate_poker_atlas", SCRIPT_PATH
)
generate_poker_atlas = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generate_poker_atlas)


def run_main(*arguments):
    old_argv = sys.argv
    sys.argv = ["generate_poker_atlas.py", *arguments]
    try:
        return generate_poker_atlas.main()
    finally:
        sys.argv = old_argv


def opaque_pixels(pixels, width, slot):
    left, top = generate_poker_atlas.slot_origin(slot)
    drawn = 0

    for py in range(generate_poker_atlas.SLOT_HEIGHT):
        for px in range(generate_poker_atlas.SLOT_WIDTH):
            at = ((top + py) * width + left + px) * 4
            drawn += 1 if pixels[at + 3] > 0 else 0

    return drawn


def it_draws_an_atlas_the_header_can_address():
    width, height, _ = generate_poker_atlas.build_atlas()

    assert width == generate_poker_atlas.COLUMNS * 32
    assert height == generate_poker_atlas.ROWS * 32
    assert (
        generate_poker_atlas.FIRST_RANK_SLOT
        + generate_poker_atlas.RANK_COUNT
        <= generate_poker_atlas.COLUMNS * generate_poker_atlas.ROWS
    )


def it_draws_something_in_every_slot_the_app_addresses():
    width, _, pixels = generate_poker_atlas.build_atlas()

    fixed = (
        generate_poker_atlas.CARD_FACE_SLOT,
        generate_poker_atlas.CARD_BACK_SLOT,
        generate_poker_atlas.FELT_SLOT,
        generate_poker_atlas.PLATE_SLOT,
        generate_poker_atlas.CHAIR_SLOT,
        generate_poker_atlas.CHIP_SLOT,
        generate_poker_atlas.DEALER_SLOT,
        generate_poker_atlas.TO_ACT_SLOT,
    )

    for slot in fixed:
        assert opaque_pixels(pixels, width, slot) > 0

    for suit in range(generate_poker_atlas.SUIT_COUNT):
        slot = generate_poker_atlas.FIRST_SUIT_SLOT + suit
        assert opaque_pixels(pixels, width, slot) > 0

    for rank in range(generate_poker_atlas.RANK_COUNT):
        slot = generate_poker_atlas.FIRST_RANK_SLOT + rank
        assert opaque_pixels(pixels, width, slot) > 0


def it_draws_a_distinct_glyph_for_every_rank():
    width, _, pixels = generate_poker_atlas.build_atlas()

    seen = set()

    for rank in range(generate_poker_atlas.RANK_COUNT):
        left, top = generate_poker_atlas.slot_origin(
            generate_poker_atlas.FIRST_RANK_SLOT + rank
        )
        shape = tuple(
            pixels[((top + py) * width + left + px) * 4 + 3] > 0
            for py in range(generate_poker_atlas.SLOT_HEIGHT)
            for px in range(generate_poker_atlas.SLOT_WIDTH)
        )
        seen.add(shape)

    assert len(seen) == generate_poker_atlas.RANK_COUNT


def it_covers_the_felt_tile_edge_to_edge():
    width, _, pixels = generate_poker_atlas.build_atlas()

    # The felt is tiled across the table, so a hole would show.
    assert opaque_pixels(
        pixels, width, generate_poker_atlas.FELT_SLOT
    ) == generate_poker_atlas.SLOT_WIDTH * generate_poker_atlas.SLOT_HEIGHT


def it_leaves_the_glyph_slots_white_so_a_tint_decides_the_colour():
    width, _, pixels = generate_poker_atlas.build_atlas()

    left, top = generate_poker_atlas.slot_origin(
        generate_poker_atlas.FIRST_SUIT_SLOT
    )

    for py in range(generate_poker_atlas.SLOT_HEIGHT):
        for px in range(generate_poker_atlas.SLOT_WIDTH):
            at = ((top + py) * width + left + px) * 4
            if pixels[at + 3] > 0:
                assert tuple(pixels[at : at + 3]) == (255, 255, 255)


def it_refuses_a_slot_two_painters_share_or_one_off_the_atlas():
    capacity = generate_poker_atlas.COLUMNS * generate_poker_atlas.ROWS

    try:
        generate_poker_atlas.check_slots([0, 1, 1])
    except generate_poker_atlas.LayoutError as error:
        assert "share slot" in str(error)
    else:
        raise AssertionError("a shared slot was accepted")

    try:
        generate_poker_atlas.check_slots([0, capacity])
    except generate_poker_atlas.LayoutError as error:
        assert "outside" in str(error)
    else:
        raise AssertionError("a slot off the atlas was accepted")

    generate_poker_atlas.check_slots([0, capacity - 1])


def it_renders_the_same_bytes_every_time():
    assert generate_poker_atlas.render() == generate_poker_atlas.render()


def it_writes_a_png_and_then_reports_it_as_current():
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        assert run_main("--root", str(root)) == 0
        assert (root / generate_poker_atlas.ATLAS_PATH).exists()
        assert run_main("--root", str(root), "--check") == 0


def it_reports_a_missing_atlas():
    with tempfile.TemporaryDirectory() as directory:
        assert run_main("--root", directory, "--check") == 1


def it_reports_a_stale_atlas():
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        target = root / generate_poker_atlas.ATLAS_PATH
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(b"not a png")
        assert run_main("--root", str(root), "--check") == 1


def it_keeps_the_committed_atlas_in_step_with_the_generator():
    assert run_main("--root", str(REPO_ROOT), "--check") == 0


def main():
    tests = [
        it_draws_an_atlas_the_header_can_address,
        it_draws_something_in_every_slot_the_app_addresses,
        it_draws_a_distinct_glyph_for_every_rank,
        it_covers_the_felt_tile_edge_to_edge,
        it_leaves_the_glyph_slots_white_so_a_tint_decides_the_colour,
        it_refuses_a_slot_two_painters_share_or_one_off_the_atlas,
        it_renders_the_same_bytes_every_time,
        it_writes_a_png_and_then_reports_it_as_current,
        it_reports_a_missing_atlas,
        it_reports_a_stale_atlas,
        it_keeps_the_committed_atlas_in_step_with_the_generator,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
