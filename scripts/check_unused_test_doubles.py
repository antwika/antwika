#!/usr/bin/env python3
# Checks headers under src/**/tests/{mocks,fakes}/include.
# Fails if any of them is never #included by a .cpp file.
# That means it's dead test-double code.
import argparse
import sys
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent


def is_test_double_path(path: Path) -> bool:
    text = path.as_posix()
    return "/tests/mocks/include/" in text or "/tests/fakes/include/" in text


def find_test_doubles(root: Path) -> list[Path]:
    return sorted(p for p in (root / "src").rglob("*.hpp") if is_test_double_path(p))


def is_included_anywhere(header_name: str, root: Path) -> bool:
    for cpp_file in (root / "src").rglob("*.cpp"):
        if header_name in cpp_file.read_text(errors="ignore"):
            return True
    return False


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=DEFAULT_ROOT, help="Repository root (defaults to the parent of scripts/)")
    args = parser.parse_args()

    doubles = find_test_doubles(args.root)

    if not doubles:
        print("No test doubles found under src/**/tests/{mocks,fakes}/include -- did the layout change?", file=sys.stderr)
        return 1

    orphans = [header for header in doubles if not is_included_anywhere(header.name, args.root)]

    if orphans:
        print("The following test doubles are never included by any .cpp file:")
        for orphan in orphans:
            print(f"  - {orphan.relative_to(args.root)}")
        print()
        print("Either delete the unused test double or add a test that uses it.")
        return 1

    print(f"OK: every mock/fake header is included by at least one .cpp file ({len(doubles)} checked).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
