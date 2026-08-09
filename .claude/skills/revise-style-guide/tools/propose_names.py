#!/usr/bin/env python3
"""List each non-conforming test name with the calls its body makes.

Used to pick the method half of Method_DoesX from the code rather than
from the old name. Prints one line per test: line, old name, then the
identifiers called in the body ranked by frequency.

    python3 propose_names.py src/libs/ui src/apps/game

Print the whole output. Piping through head has hidden the tail of long
files before now, and six modules were reported clean while still
holding names.
"""

import importlib.util
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]

spec = importlib.util.spec_from_file_location(
    "gate", ROOT / "scripts/check_comment_style.py"
)
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)

NOISE = {
    "EXPECT_EQ", "EXPECT_NE", "EXPECT_TRUE", "EXPECT_FALSE", "EXPECT_THROW",
    "EXPECT_GT", "EXPECT_LT", "EXPECT_GE", "EXPECT_LE", "EXPECT_STREQ",
    "EXPECT_NO_THROW", "EXPECT_DOUBLE_EQ", "EXPECT_FLOAT_EQ", "EXPECT_CALL",
    "ASSERT_EQ", "ASSERT_NE", "ASSERT_TRUE", "ASSERT_FALSE", "ASSERT_THROW",
    "ASSERT_GT", "ASSERT_LT", "SUCCEED", "FAIL", "TEST", "TEST_F", "TEST_P",
    "std", "vector", "string", "size_t", "uint64_t", "int64_t", "move",
    "make_unique", "make_shared", "optional", "nullopt", "size", "push_back",
    "emplace_back", "static_cast", "reinterpret_cast", "const_cast",
    "if", "for", "while", "switch", "return", "sizeof", "value", "has_value",
}

CALL = re.compile(r"\b([a-z][A-Za-z0-9_]*)\s*\(")


def bodies(text):
    out = []
    for match in gate.TEST_MACRO.finditer(text):
        name = match.group(1)
        start = text.find("{", match.end())
        if start == -1:
            continue
        depth = 0
        i = start
        while i < len(text):
            if text[i] == "{":
                depth += 1
            elif text[i] == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        line = text.count("\n", 0, match.start(1)) + 1
        out.append((line, name, text[start:i]))
    return out


def main():
    for module in sys.argv[1:]:
        for path in sorted(Path(module).rglob("*.cpp")):
            text = path.read_text(errors="ignore")
            bad = {n for _, n in gate.find_ungrammatical_test_names(text)}
            if not bad:
                continue
            print(f"\n### {path}")
            for line, name, body in bodies(text):
                if name not in bad:
                    continue
                calls = Counter(
                    c for c in CALL.findall(body) if c not in NOISE
                )
                top = ", ".join(c for c, _ in calls.most_common(4))
                print(f"{line}\t{name}\t[{top}]")


if __name__ == "__main__":
    sys.exit(main())
