#!/usr/bin/env python3

import argparse
import re
import sys
from pathlib import Path

INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)

OFFSET_BASIS = 14695981039346656037
PRIME = 1099511628211
MASK = (1 << 64) - 1


def roots_under(root: Path) -> list[Path]:
    return sorted(
        path.resolve()
        for path in root.glob("src/libs/*/include")
        if path.is_dir()
    )


def resolve(name: str, roots: list[Path]) -> Path | None:
    for one in roots:
        found = one / name

        if found.is_file():
            return found

    return None


def closure(head: Path, roots: list[Path]) -> list[Path]:
    seen: set[Path] = set()
    going = [head]

    while going:
        path = going.pop()

        if path in seen:
            continue

        seen.add(path)

        for name in INCLUDE.findall(path.read_text(encoding="utf-8")):
            found = resolve(name, roots)

            if found is not None:
                going.append(found)

    return sorted(seen)


def stamp_of(paths: list[Path], root: Path) -> int:
    out = OFFSET_BASIS

    for path in paths:
        text = path.relative_to(root).as_posix() + "\0"
        text += path.read_text(encoding="utf-8")

        for letter in text.encode("utf-8"):
            out = ((out ^ letter) * PRIME) & MASK

    return out


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--header", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    roots = roots_under(args.root)
    paths = closure(args.header.resolve(), roots)
    stamp = stamp_of(paths, args.root.resolve())

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        "#pragma once\n"
        "\n"
        "#include <cstdint>\n"
        "\n"
        "namespace antwika::gameplay\n"
        "{\n"
        "\n"
        "    /**\n"
        "     * @brief Hash of the headers reachable from\n"
        "     *        IGame.hpp, taken when this was built.\n"
        "     *\n"
        "     * The host compares its own value with the one the\n"
        "     * loaded module exports.\n"
        "     * A mismatch means the two were built against\n"
        "     * different headers, so the module is rejected.\n"
        "     */\n"
        f"    inline constexpr std::uint64_t kSeamStamp = {stamp}ULL;\n"
        "\n"
        "}\n",
        encoding="utf-8",
    )

    return 0


if __name__ == "__main__":
    sys.exit(main())
