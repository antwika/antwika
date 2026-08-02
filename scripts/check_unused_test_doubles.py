#!/usr/bin/env python3
# Checks headers under src/**/tests/{mocks,fakes}/include.
# Fails if any of them is never #included by a .cpp file.
# A .cpp under src/ or under backends/ counts, since both hold suites.
# That means it's dead test-double code.
import argparse
import re
import sys
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent

INCLUDE_PATTERN = re.compile(r'#include\s*[<"]([^">]+)[">]')


def is_test_double_path(path: Path) -> bool:
    text = path.as_posix()
    return "/tests/mocks/include/" in text or "/tests/fakes/include/" in text


def find_test_doubles(root: Path) -> list[Path]:
    headers = (root / "src").rglob("*.hpp")
    return sorted(p for p in headers if is_test_double_path(p))


def include_path_for(header: Path) -> str:
    # The path a consumer actually writes inside #include, e.g.
    # "antwika/foo/mocks/MockFoo.hpp".
    # That's every segment after the header's own include/ directory.
    parts = header.parts
    index = parts.index("include")
    return "/".join(parts[index + 1 :])


def collect_included_paths(root: Path) -> set[str]:
    # Every path any .cpp under src/ or backends/ includes, in one pass.
    # Asking per test double re-read the whole tree once per double.
    # backends/ counts because its conformance suites are real consumers.
    # A double used only there was reported as an orphan while it did not.
    included: set[str] = set()

    for directory in ("src", "backends"):
        for cpp_file in (root / directory).rglob("*.cpp"):
            text = cpp_file.read_text(errors="ignore")
            included.update(
                match.group(1) for match in INCLUDE_PATTERN.finditer(text)
            )

    return included


def is_included_anywhere(include_path: str, included: set[str]) -> bool:
    # Takes the collected set rather than a root to re-scan per double.
    # That is what lets main() call this instead of inlining a copy.
    return include_path in included


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=DEFAULT_ROOT,
        help="Repository root (defaults to the parent of scripts/)",
    )
    args = parser.parse_args()

    doubles = find_test_doubles(args.root)

    if not doubles:
        print(
            "No test doubles found under src/**/tests/{mocks,fakes}/include "
            "-- did the layout change?",
            file=sys.stderr,
        )
        return 1

    included = collect_included_paths(args.root)

    orphans = [
        header
        for header in doubles
        if not is_included_anywhere(include_path_for(header), included)
    ]

    if orphans:
        print("The following test doubles are never included by any .cpp file:")
        for orphan in orphans:
            print(f"  - {orphan.relative_to(args.root)}")
        print()
        print("Either delete the unused test double, or add a test using it.")
        return 1

    print(
        f"OK: every mock/fake header is included by at least one .cpp file "
        f"({len(doubles)} checked)."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
