#!/usr/bin/env python3
# Fails if README.md's project tree disagrees with what the tree builds.
# CMake knows which modules exist, so the lists come from its files.
# Every application also has to appear in README's list of binaries.
# README drifted by five libraries and one application unnoticed.
# That is the argument for checking this rather than re-reading it.
import argparse
import re
import sys
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent

ADD_SUBDIRECTORY = re.compile(r"^\s*add_subdirectory\(([^)\s]+)\)")
FENCE = re.compile(r"^(```|~~~)")
# A tree entry is four columns of indent per level, then a branch glyph.
TREE_ENTRY = re.compile(r"^((?:[│ ]   )*)(?:├──|└──) ([^\s/]+)/$")
TREE_ROOT = re.compile(r"^([^\s/]+)/$")


def subdirectories_of(cmake_file: Path) -> list[str]:
    lines = cmake_file.read_text(encoding="utf-8").splitlines()
    names = []
    for line in lines:
        match = ADD_SUBDIRECTORY.match(line)
        if match:
            names.append(match.group(1))
    return sorted(names)


def backend_directories(root: Path) -> list[str]:
    backends = root / "backends"
    return sorted(
        path.name
        for path in backends.iterdir()
        if path.is_dir() and (path / "CMakeLists.txt").is_file()
    )


def tree_children(readme: Path) -> dict[str, list[str]]:
    # Maps a directory named in README's tree to what is drawn under it.
    # Only fenced blocks are read, so prose naming a module is not enough.
    children: dict[str, set[str]] = {}
    stack: list[str] = []
    in_fence = False

    for raw in readme.read_text(encoding="utf-8").splitlines():
        if FENCE.match(raw.strip()):
            in_fence = not in_fence
            stack = []
            continue
        if not in_fence:
            continue

        root_match = TREE_ROOT.match(raw)
        if root_match:
            stack = [root_match.group(1)]
            continue

        entry = TREE_ENTRY.match(raw)
        if not entry:
            continue

        depth = len(entry.group(1)) // 4
        if depth >= len(stack):
            continue

        name = entry.group(2)
        children.setdefault(stack[depth], set()).add(name)
        stack = stack[: depth + 1] + [name]

    return {parent: sorted(names) for parent, names in children.items()}


def compare(label: str, expected: list[str], listed: list[str]) -> list[str]:
    problems = []
    for name in expected:
        if name not in listed:
            problems.append(f"{label}: '{name}' is missing from README.md")
    for name in listed:
        if name not in expected:
            problems.append(f"{label}: README.md lists '{name}', which is gone")
    return problems


def find_problems(root: Path) -> list[str]:
    readme = root / "README.md"
    children = tree_children(readme)
    text = readme.read_text(encoding="utf-8")

    libs = subdirectories_of(root / "src/libs/CMakeLists.txt")
    apps = subdirectories_of(root / "src/apps/CMakeLists.txt")
    backends = backend_directories(root)

    problems = []
    problems += compare("src/libs", libs, children.get("libs", []))
    problems += compare("src/apps", apps, children.get("apps", []))
    problems += compare("backends", backends, children.get("backends", []))

    # Every application earns a bullet naming the binary it builds.
    # A new one cannot ship with nothing saying how to run it.
    for app in apps:
        binary = f"build/bin/antwika_{app}/antwika_{app}"
        if binary not in text:
            problems.append(
                f"binaries: README.md never names '{binary}'"
            )

    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=DEFAULT_ROOT,
        help="Repository root (defaults to the parent of scripts/)",
    )
    args = parser.parse_args()

    problems = find_problems(args.root)

    if problems:
        print(f"Found {len(problems)} README.md drift(s):")
        print()
        for problem in problems:
            print(f"  - {problem}")
        print()
        print("Update README.md's project tree and its list of binaries.")
        return 1

    print(
        "OK: README.md names every library, application and backend the "
        "tree builds."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
