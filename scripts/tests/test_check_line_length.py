#!/usr/bin/env python3
# Plain-assert tests for check_line_length.py.
# Run directly:
#   python3 scripts/tests/test_check_line_length.py
import importlib.util
import tempfile
from pathlib import Path

SCRIPT_PATH = Path(__file__).resolve().parent.parent / "check_line_length.py"

spec = importlib.util.spec_from_file_location(
    "check_line_length", SCRIPT_PATH
)
check_line_length = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check_line_length)


def write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)


def it_allows_a_line_at_exactly_eighty_characters():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "Foo.cpp"
        write(path, ("x" * 80) + "\n")

        assert check_line_length.find_long_lines(path) == []


def it_flags_a_line_over_eighty_characters():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "Foo.cpp"
        write(path, ("x" * 81) + "\n")

        violations = check_line_length.find_long_lines(path)

        assert violations == [(1, 81)]


def it_reports_the_correct_line_number_among_several_lines():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "Foo.cpp"
        write(path, "short\n" + ("y" * 90) + "\nshort again\n")

        violations = check_line_length.find_long_lines(path)

        assert violations == [(2, 90)]


def it_finds_violations_across_the_configured_file_globs():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "src/libs/foo/src/Foo.cpp", ("a" * 90) + "\n")
        write(root / "scripts/check_foo.py", "short\n")

        violations = check_line_length.find_violations(root)

        assert len(violations) == 1
        assert violations[0][0] == root / "src/libs/foo/src/Foo.cpp"


def it_checks_backend_sources_outside_src():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "backends/sdl3/src/Sdl3Backend.cpp", ("a" * 90) + "\n")
        write(root / "backends/sdl3/src/Sdl3Backend.hpp", ("b" * 90) + "\n")

        violations = check_line_length.find_violations(root)

        assert len(violations) == 2
        assert violations[0][1] == 1
        assert violations[0][2] == 90


def it_keeps_every_configured_source_line_at_or_under_eighty_chars():
    root = check_line_length.DEFAULT_ROOT

    violations = check_line_length.find_violations(root)

    details = [f"{p}:{n}: {length} chars" for p, n, length in violations]
    assert details == [], "\n".join(details)


def main():
    tests = [
        it_allows_a_line_at_exactly_eighty_characters,
        it_flags_a_line_over_eighty_characters,
        it_reports_the_correct_line_number_among_several_lines,
        it_finds_violations_across_the_configured_file_globs,
        it_checks_backend_sources_outside_src,
        it_keeps_every_configured_source_line_at_or_under_eighty_chars,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
