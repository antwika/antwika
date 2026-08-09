#!/usr/bin/env python3
"""Wrap TEST macro lines that a rename pushed past eighty columns.

    python3 wrap_test_lines.py src backends

A longer name makes a longer line, and check_line_length.py is what
notices. Three separate rename batches hit this before it was worth
scripting. The wrap puts the fixture on the first line and the name on
the second, indented five spaces, which is the layout the guide shows.

A name that still will not fit after wrapping is reported rather than
cut, and has to be shortened by hand.
"""

import re
import sys
from pathlib import Path

ONE_LINE = re.compile(
    r"^(\s*)((?:TYPED_)?TEST(?:_F|_P)?)"
    r"\(\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\)(.*)$"
)


def wrap(paths):
    wrapped = 0
    stubborn = []
    for path in paths:
        lines = Path(path).read_text().split("\n")
        out = []
        changed = False
        for line in lines:
            if len(line) <= 80:
                out.append(line)
                continue
            match = ONE_LINE.match(line)
            if not match:
                out.append(line)
                continue
            indent, macro, fixture, name, tail = match.groups()
            under = " " * (len(macro) + 1)
            second = f"{indent}{under}{name}){tail}"
            if len(second) > 80:
                stubborn.append((str(path), name, len(name)))
                out.append(line)
                continue
            out.append(f"{indent}{macro}({fixture},")
            out.append(second)
            changed = True
            wrapped += 1
        if changed:
            Path(path).write_text("\n".join(out))
    print(f"wrapped {wrapped}")
    for path, name, length in stubborn:
        print(f"  STILL LONG {Path(path).name}: {name} ({length})")
    return wrapped


def main():
    paths = []
    for root in sys.argv[1:]:
        paths.extend(Path(root).rglob("*.cpp"))
        paths.extend(Path(root).rglob("*.hpp"))
    wrap(paths)


if __name__ == "__main__":
    sys.exit(main())
