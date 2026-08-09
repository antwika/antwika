#!/usr/bin/env python3
"""Rename gtest test names, refusing anything unsafe.

Import and call one of the two functions. Both refuse a name over the
length limit and both report rather than guess.

    from rename_tests import rename_unique, rename_every

    rename_unique({
        "src/libs/a/tests/ATest.cpp": [("OldName", "New_Name")],
    })

rename_unique is the default: it rewrites a name only where exactly one
TEST macro carries it, so a typo cannot silently hit the wrong test. It
matches a macro wrapped across lines at any indent, which a plain string
replace does not.

rename_every rewrites every occurrence in a file, for the suites that
repeat one name across several fixtures. ValueEqualityTest and
CityGridTest both do this; rename_unique cannot touch them.

Two things this deliberately does not do. It never truncates a name to
fit the limit, because a cut name can collide with another in the same
fixture and the collision only shows up as a link error much later. It
never checks name uniqueness across the whole tree, because a fixture
name is not global to the repository: seven separate modules define a
StateDumpTest, and each is its own binary.
"""

import re
from pathlib import Path

MAX_LENGTH = 75


def rename_unique(table):
    total = 0
    for name, pairs in table.items():
        path = Path(name)
        text = path.read_text()
        for old, new in pairs:
            if len(new) > MAX_LENGTH:
                print(f"TOO LONG {name}: {new} ({len(new)})")
                continue
            pattern = re.compile(r"(,\s*)" + re.escape(old) + r"(\s*\))")
            if len(pattern.findall(text)) != 1:
                print(f"SKIP {name}: {old}")
                continue
            text = pattern.sub(
                lambda mo: mo.group(1) + new + mo.group(2), text
            )
            total += 1
        path.write_text(text)
    print(f"renamed {total}")
    return total


def rename_every(table):
    total = 0
    for name, pairs in table.items():
        path = Path(name)
        text = path.read_text()
        for old, new in pairs:
            if len(new) > MAX_LENGTH:
                print(f"TOO LONG {name}: {new} ({len(new)})")
                continue
            pattern = re.compile(r"(,\s*)" + re.escape(old) + r"(\s*\))")
            total += len(pattern.findall(text))
            text = pattern.sub(
                lambda mo: mo.group(1) + new + mo.group(2), text
            )
        path.write_text(text)
    print(f"renamed {total}")
    return total
