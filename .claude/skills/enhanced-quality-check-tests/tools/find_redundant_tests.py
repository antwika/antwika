"""Report tests another test already covers.

This is the static, cheapest layer of the redundancy question. It finds
candidates only. A candidate is not a verdict — see SKILL.md for the
coverage and mutation layers that decide.

  identical bodies, ignoring whitespace;

  one test's assertions are a subset of another's in the same file, with
  the same subject.

    python3 find_redundant_tests.py [--root DIR]
"""
import argparse
import collections
import hashlib
import sys

import testblocks as tb

IDENTICAL = "identical body"
SUBSUMED = "assertions are a subset of another test in the same file"

TOO_SHORT = 40


def assertion_set(body):
    """The normalised assertions in one body, as a comparable set."""
    out = set()
    for macro, args, _ in tb.macro_calls(body):
        out.add(macro + "(" + ",".join(args) + ")")
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    options = parser.parse_args()

    counts = collections.Counter()
    by_body = collections.defaultdict(list)

    for path, relative in tb.test_files(options.root):
        text = path.read_text(errors="replace")
        tests = list(tb.test_blocks(text))

        for _, name, body, line in tests:
            normalised = " ".join(body.split())
            if len(normalised) < TOO_SHORT:
                continue
            digest = hashlib.md5(normalised.encode()).hexdigest()
            by_body[digest].append(f"{relative}:{line} {name}")

        assertions = []
        for _, name, body, line in tests:
            found = assertion_set(body)
            if len(found) >= 2:
                assertions.append((name, line, found))

        for name, line, found in assertions:
            for other, _, others in assertions:
                if other == name or not found < others:
                    continue
                counts[SUBSUMED] += 1
                print(
                    f"{relative}:{line} | {name} | {SUBSUMED} | "
                    f"covered by {other}")
                break

    for digest, where in sorted(by_body.items()):
        if len(where) < 2:
            continue
        counts[IDENTICAL] += len(where) - 1
        print(f"{IDENTICAL} x{len(where)}:")
        for entry in where:
            print(f"    {entry}")

    print()
    for category, count in sorted(counts.items()):
        print(f"{count:5d}  {category}")
    print(f"{sum(counts.values()):5d}  redundancy candidates")
    return 1 if counts else 0


if __name__ == "__main__":
    sys.exit(main())
