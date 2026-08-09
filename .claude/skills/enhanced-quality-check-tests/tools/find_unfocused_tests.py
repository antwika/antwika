"""Report tests that are about more than one thing.

The repository's `Method_Behaviour` naming makes the claimed subject
machine-readable, so focus is checkable rather than a matter of taste.

  the assertions are about several unrelated objects, so the test fails
  for reasons its name does not cover.

The name-claims-an-uncalled-method check was measured and dropped: on
this tree it fired on 22% of all tests, because a name such as
`Describe_...` reaches its subject through a scene object rather than a
call spelled `describe`. A detector that noisy teaches people to ignore
it, which is the failure this skill exists to prevent. Judge name/body
agreement by reading, not by grep.

Asserting many fields of one value is one thing; asserting a value, a
log and a file is three.

    python3 find_unfocused_tests.py [--root DIR] [--subjects N]
"""
import argparse
import collections
import re
import sys

import testblocks as tb

MANY_SUBJECTS = "assertions are about several unrelated objects"

NOISE = {
    "EXPECT", "ASSERT", "true", "false", "nullptr", "std", "testing",
    "antwika", "expected", "result", "results", "actual", "out", "value",
}


def subjects_of(body):
    """The distinct root objects the assertions are about."""
    roots = collections.Counter()
    for macro, args, _ in tb.macro_calls(body):
        if macro == "EXPECT_CALL" or not args:
            continue
        root = tb.root_identifier(args[0])
        if root and root not in NOISE and not root.startswith("k"):
            roots[root] += 1
    return roots


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    parser.add_argument("--subjects", type=int, default=4)
    options = parser.parse_args()

    counts = collections.Counter()
    for path, relative in tb.test_files(options.root):
        text = path.read_text(errors="replace")
        for _, name, body, line in tb.test_blocks(text):
            roots = subjects_of(body)
            if len(roots) >= options.subjects:
                counts[MANY_SUBJECTS] += 1
                listed = ", ".join(sorted(roots))[:70]
                print(
                    f"{relative}:{line} | {name} | {MANY_SUBJECTS} | "
                    f"{len(roots)}: {listed}")

    print()
    for category, count in sorted(counts.items()):
        print(f"{count:5d}  {category}")
    print(f"{sum(counts.values()):5d}  total")
    return 1 if counts else 0


if __name__ == "__main__":
    sys.exit(main())
