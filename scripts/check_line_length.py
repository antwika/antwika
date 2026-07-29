#!/usr/bin/env python3
# Fails if a source line is longer than 80 characters.
import argparse
import sys
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent

CPP_GLOBS = ("src/**/*.cpp", "src/**/*.hpp")
PYTHON_GLOBS = ("scripts/*.py", "scripts/tests/*.py")

MAX_LINE_LENGTH = 80


def find_long_lines(path: Path) -> list[tuple[int, int]]:
    violations = []
    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    for line_no, raw in enumerate(lines, start=1):
        if len(raw) > MAX_LINE_LENGTH:
            violations.append((line_no, len(raw)))
    return violations


def find_violations(root: Path) -> list[tuple[Path, int, int]]:
    violations: list[tuple[Path, int, int]] = []

    for pattern in CPP_GLOBS + PYTHON_GLOBS:
        for path in sorted(root.glob(pattern)):
            for line_no, length in find_long_lines(path):
                violations.append((path, line_no, length))

    return violations


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=DEFAULT_ROOT,
        help="Repository root (defaults to the parent of scripts/)",
    )
    args = parser.parse_args()

    violations = find_violations(args.root)

    if violations:
        print(f"Found {len(violations)} line(s) over {MAX_LINE_LENGTH} chars:")
        print()
        for path, line_no, length in violations:
            print(f"{path}:{line_no}: {length} chars")
        print()
        print(f"Wrap or shorten these to {MAX_LINE_LENGTH} chars or fewer.")
        return 1

    print(f"OK: every checked line is at most {MAX_LINE_LENGTH} characters.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
