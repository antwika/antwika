"""Report tests that cannot fail for the reason their name gives.

Covers the mechanical half of the checklist: a computation compared to
itself, a body with no assertion, two runs compared to each other, a
value the double supplied, and a comparison blind to identity.

    python3 find_unfailable_tests.py [--root DIR]
"""
import argparse
import collections
import re
import sys

import testblocks as tb

SELF_COMPARE = "compares a computation to itself"
NO_ASSERTION = "body contains no assertion"
RUN_TWICE = "two runs of one computation compared to each other"
DOUBLE_ANSWERS = "asserts the value the double was told to return"
IDENTITY_BLIND = "an object compared to itself, blind to identity equality"
TIMES_ANY = "every expectation is Times(AnyNumber()), so none can fail"
SOFT_PRECONDITION = "EXPECT_ on a precondition that is then dereferenced"
NO_ACT = "the subject is built but never exercised"

TIMES_ANY_RE = re.compile(
    r"\.Times\s*\(\s*(?:::)?(?:testing::)?AnyNumber\s*\(\s*\)\s*\)")

HAS_VALUE = re.compile(
    r"EXPECT_TRUE\s*\(\s*([A-Za-z_]\w*)(?:\.\w+\(\))*\.has_value\(\)")

CANNED = re.compile(
    r"Will(?:ByDefault|Once|Repeatedly)\s*\(\s*Return\s*\(([^;]{1,80}?)\)\s*\)")
INITIALISER = re.compile(
    r"\b(?:const\s+)?auto\s*&?\s*(\w+)\s*=\s*([^;]+);")


OPERATOR_SUBJECT = re.compile(r"^Operator(Equals|Compare)_")


def findings_for(body, name):
    """Yield (category, detail) for one test body.

    A test named OperatorEquals_* has the comparison itself as its
    subject, so two independently built equal values is the shape that
    test must have, not a smell.
    """
    comparison_is_the_subject = bool(OPERATOR_SUBJECT.match(name))
    if not tb.ASSERT_RE.search(body):
        yield NO_ASSERTION, ""

    for macro, args, _ in tb.macro_calls(body, ("EXPECT_EQ", "ASSERT_EQ")):
        if len(args) < 2 or not args[0] or args[0] != args[1]:
            continue
        if args[0].startswith("&"):
            continue
        built_twice = "(" in args[0] or "{" in args[0]
        if built_twice and comparison_is_the_subject:
            continue
        if built_twice:
            yield SELF_COMPARE, f"{macro}({args[0]}, {args[1]})"
        else:
            yield IDENTITY_BLIND, f"{macro}({args[0]}, {args[1]})"

    twins = collections.defaultdict(list)
    for m in INITIALISER.finditer(body):
        twins[" ".join(m.group(2).split())].append(m.group(1))
    twins = {k: v for k, v in twins.items() if len(v) > 1}
    if twins:
        for macro, args, _ in tb.macro_calls(body, ("EXPECT_EQ", "ASSERT_EQ")):
            if len(args) < 2:
                continue
            left = tb.root_identifier(args[0])
            right = tb.root_identifier(args[1])
            for expression, names in twins.items():
                if left in names and right in names and left != right:
                    yield RUN_TWICE, f"{args[0]} == {args[1]} (both from " \
                                     f"{expression[:48]})"

    expectations = body.count("EXPECT_CALL")
    permissive = len(TIMES_ANY_RE.findall(body))
    if expectations and permissive == expectations:
        strict = re.search(
            r"\.(WillOnce|WillRepeatedly|InSequence)|InSequence\s+\w", body)
        if not strict:
            yield TIMES_ANY, f"{permissive} of {expectations} expectations"

    for m in HAS_VALUE.finditer(body):
        name_of = m.group(1)
        after = body[m.end():]
        deref = re.search(
            r"(\*\s*" + re.escape(name_of) + r"\b|\b"
            + re.escape(name_of) + r"\s*(?:->|\.value\(\)))", after)
        if deref:
            yield SOFT_PRECONDITION, f"`{name_of}` — use ASSERT_TRUE"

    canned = set()
    for m in CANNED.finditer(body):
        value = " ".join(m.group(1).split())
        if value and value not in ("true", "false") and len(value) > 3:
            canned.add(value)
    if canned:
        for macro, args, _ in tb.macro_calls(body, ("EXPECT_EQ", "ASSERT_EQ")):
            joined = " ".join(args[:2])
            for value in canned:
                if value in joined:
                    yield DOUBLE_ANSWERS, f"canned `{value}`"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    options = parser.parse_args()

    counts = collections.Counter()
    for path, relative in tb.test_files(options.root):
        text = path.read_text(errors="replace")
        for _, name, body, line in tb.test_blocks(text):
            for category, detail in findings_for(body, name):
                counts[category] += 1
                suffix = f" | {detail}" if detail else ""
                print(f"{relative}:{line} | {name} | {category}{suffix}")

    print()
    for category, count in sorted(counts.items()):
        print(f"{count:5d}  {category}")
    print(f"{sum(counts.values()):5d}  total")
    return 1 if counts else 0


if __name__ == "__main__":
    sys.exit(main())
