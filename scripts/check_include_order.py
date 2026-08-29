#!/usr/bin/env python3

import argparse
import re
import sys
from pathlib import Path

from repofiles import DEFAULT_ROOT, CPP_GLOBS, find_paths


INCLUDE = re.compile(r"^[ \t]*#[ \t]*include[ \t]+([<\"][^>\"]+[>\"])")

CONDITIONAL_OPENER = re.compile(r"#\s*(?:if|ifdef|ifndef)\b")

CONDITIONAL_CLOSER = re.compile(r"#\s*endif\b")

CONDITIONAL_BRANCH = re.compile(r"#\s*(?:else|elif)\b")

PRAGMA_ONCE = re.compile(r"#\s*pragma\s+once\b")

INCLUDE_GROUPS = (
    "own",
    "third-party",
    "std",
    "project-angled",
    "project-quoted",
)

OUT_OF_ORDER = "include group out of order"

MISSING_SEPARATOR = "missing blank line between include groups"


def include_group(include: str, stem: str) -> str:
    body = include[1:-1]

    if include.startswith('"'):
        if body == f"{stem}.hpp" or body.endswith(f"/{stem}.hpp"):
            return "own"
        return "project-quoted"

    if "/" not in body:
        return "std"

    if body.startswith("antwika/"):
        return "project-angled"

    return "third-party"


def find_include_violations(text: str, stem: str) -> list[tuple[int, str]]:
    found: list[tuple[int, str]] = []
    reached = 0
    depth = 0
    previous: str | None = None
    blank_before = True

    for number, line in enumerate(text.split("\n"), start=1):
        stripped = line.strip()

        if not stripped:
            blank_before = True
            continue

        if CONDITIONAL_OPENER.match(stripped):
            depth += 1
            continue

        if CONDITIONAL_CLOSER.match(stripped):
            depth = max(0, depth - 1)
            continue

        if CONDITIONAL_BRANCH.match(stripped):
            continue

        if depth:
            continue

        if PRAGMA_ONCE.match(stripped):
            blank_before = False
            continue

        match = INCLUDE.match(line)

        if not match:
            break

        group = include_group(match.group(1), stem)
        rank = INCLUDE_GROUPS.index(group)

        if rank < reached:
            found.append((number, OUT_OF_ORDER))

        reached = max(reached, rank)

        if previous is not None and group != previous and not blank_before:
            found.append((number, MISSING_SEPARATOR))

        previous = group
        blank_before = False

    return sorted(found)


def find_violations(root: Path) -> list[tuple[Path, int, str]]:
    violations: list[tuple[Path, int, str]] = []

    for pattern in CPP_GLOBS:
        for path in find_paths(root, pattern):
            text = path.read_text(encoding="utf-8", errors="ignore")

            for line, rule in find_include_violations(text, path.stem):
                violations.append((path, line, rule))

    return violations


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--root",
        type=Path,
        default=DEFAULT_ROOT,
        help="Repository root (defaults to the parent of scripts/)",
    )
    args = parser.parse_args()

    violations = find_violations(args.root)

    if violations:
        print(f"Found {len(violations)} include-order violation(s):")
        print()
        for path, line, rule in violations:
            print(f"{path}:{line}: {rule}")
        print()
        print(
            "Order the leading include block as: own header, "
            "third-party, std, <antwika/...>, then quoted local "
            "includes, with a blank line between groups."
        )
        return 1

    print("OK: every leading include block follows the house order.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
