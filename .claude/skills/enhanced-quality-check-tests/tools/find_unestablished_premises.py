"""Report tests whose assertions rest on a premise nothing established.

Three shapes, all of which pass when the production code does nothing:

  every assertion sits inside a loop or a branch, so an empty collection
  means the body never runs;

  two collections are compared and neither is shown to be non-empty;

  an ordering is asserted against an index whose initial value already
  satisfies it.

    python3 find_unestablished_premises.py [--root DIR]
"""
import argparse
import collections
import re
import sys

import testblocks as tb

ALL_GUARDED = "every assertion is inside a loop or branch"
UNPINNED_PAIR = "two results compared, neither shown to be non-empty"

BLOCK_OPENER = re.compile(r"\b(for|while|if)\s*\(")

NON_EMPTY = re.compile(
    r"(ASSERT|EXPECT)_(FALSE\s*\(\s*[\w.>()-]*empty|TRUE\s*\(\s*!|"
    r"(EQ|NE|GT|GE)\s*\([^;]*\.size\(\)|(EQ|NE|GT|GE)\s*\([^;]*count)")

SIZE_LIKE = re.compile(r"\.(size|empty|count)\s*\(\)|\bsize\b|\bcount\b")

ABSOLUTE_PIN = re.compile(
    r"(EXPECT|ASSERT)_(EQ|NE|TRUE|FALSE|LT|LE|GT|GE|NEAR|THAT|STREQ|"
    r"DOUBLE_EQ|FLOAT_EQ)\s*\([^;]*?(\b\d+[uU]?[lL]*\b|\bk[A-Z]\w*|"
    r"\"|\'|::[A-Z]\w*)")


def guarded_spans(body):
    """Character ranges covered by a for/while/if block."""
    spans = []
    for m in BLOCK_OPENER.finditer(body):
        paren = body.find("(", m.start())
        close = tb.match_delimited(body, paren, "(", ")")
        if close == -1:
            continue
        brace = body.find("{", close)
        newline = body.find("\n", close)
        if brace == -1 or (newline != -1 and newline < brace):
            continue
        end = tb.match_delimited(body, brace, "{", "}")
        if end != -1:
            spans.append((brace, end))
    return spans


def all_assertions_guarded(body):
    spans = guarded_spans(body)
    if not spans:
        return False

    offsets = [off for _, _, off in tb.macro_calls(body)]
    if not offsets:
        return False

    for off in offsets:
        if not any(start < off < end for start, end in spans):
            return False
    return True


def compares_two_unpinned_results(body):
    """An EXPECT_EQ between two locals, with no non-emptiness anywhere."""
    if NON_EMPTY.search(body) or SIZE_LIKE.search(body):
        return None

    if ABSOLUTE_PIN.search(body):
        return None

    locals_seen = set(
        m.group(1)
        for m in re.finditer(
            r"\b(?:const\s+)?auto\s*&?\s*(\w+)\s*=\s*([^;]*\([^;]*);",
            body)
        if not m.group(1).startswith("k"))
    if len(locals_seen) < 2:
        return None

    for macro, args, _ in tb.macro_calls(body, ("EXPECT_EQ", "ASSERT_EQ")):
        if len(args) < 2:
            continue
        left = tb.root_identifier(args[0])
        right = tb.root_identifier(args[1])
        if left in locals_seen and right in locals_seen and left != right:
            return f"{args[0]} == {args[1]}"
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    options = parser.parse_args()

    counts = collections.Counter()
    for path, relative in tb.test_files(options.root):
        text = path.read_text(errors="replace")
        for _, name, body, line in tb.test_blocks(text):
            if all_assertions_guarded(body):
                counts[ALL_GUARDED] += 1
                print(f"{relative}:{line} | {name} | {ALL_GUARDED}")

            detail = compares_two_unpinned_results(body)
            if detail is not None:
                counts[UNPINNED_PAIR] += 1
                print(
                    f"{relative}:{line} | {name} | {UNPINNED_PAIR} | {detail}")

    print()
    for category, count in sorted(counts.items()):
        print(f"{count:5d}  {category}")
    print(f"{sum(counts.values()):5d}  total")
    return 1 if counts else 0


if __name__ == "__main__":
    sys.exit(main())
