#!/usr/bin/env python3

import importlib.util
import sys
import tempfile
from pathlib import Path

SCRIPT_PATH = Path(__file__).resolve().parent.parent / "check_line_length.py"

sys.path.insert(0, str(SCRIPT_PATH.parent))

spec = importlib.util.spec_from_file_location(
    "check_line_length", SCRIPT_PATH
)
check_line_length = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check_line_length)


def write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)


def it_allows_a_line_at_exactly_a_hundred_characters() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "Foo.cpp"
        write(path, ("x" * 100) + "\n")

        assert check_line_length.find_long_lines(path) == []


def it_flags_a_line_over_a_hundred_characters() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "Foo.cpp"
        write(path, ("x" * 101) + "\n")

        violations = check_line_length.find_long_lines(path)

        assert violations == [(1, 101)]


def it_reports_the_correct_line_number_among_several_lines() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "Foo.cpp"
        write(path, "short\n" + ("y" * 110) + "\nshort again\n")

        violations = check_line_length.find_long_lines(path)

        assert violations == [(2, 110)]


def it_finds_violations_across_the_configured_file_globs() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "src/libs/foo/src/Foo.cpp", ("a" * 110) + "\n")
        write(root / "scripts/check_foo.py", "short\n")

        violations = check_line_length.find_violations(root)

        assert len(violations) == 1
        assert violations[0][0] == root / "src/libs/foo/src/Foo.cpp"
        assert violations[0][1] == 1
        assert violations[0][2] == 110


def it_checks_backend_sources_outside_src() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "backends/raylib/src/RaylibBackend.cpp", ("a" * 110) + "\n")
        write(root / "backends/raylib/src/RaylibBackend.hpp", ("b" * 110) + "\n")

        violations = check_line_length.find_violations(root)

        assert len(violations) == 2
        assert violations[0][0] == (
            root / "backends/raylib/src/RaylibBackend.cpp"
        )
        assert violations[1][0] == (
            root / "backends/raylib/src/RaylibBackend.hpp"
        )


def it_keeps_every_configured_source_line_at_or_under_a_hundred_chars() -> None:
    root = check_line_length.DEFAULT_ROOT

    violations = check_line_length.find_violations(root)

    details = [f"{p}:{n}: {length} chars" for p, n, length in violations]
    assert details == [], "\n".join(details)


def main() -> None:
    tests = [
        it_allows_a_line_at_exactly_a_hundred_characters,
        it_flags_a_line_over_a_hundred_characters,
        it_reports_the_correct_line_number_among_several_lines,
        it_finds_violations_across_the_configured_file_globs,
        it_checks_backend_sources_outside_src,
        it_keeps_every_configured_source_line_at_or_under_a_hundred_chars,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
