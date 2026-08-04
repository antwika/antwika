#!/usr/bin/env python3
# Fails unless a gcovr --json-summary report is 100% covered.
# Checks lines, functions, and branches, each independently.
#
# The counts are compared rather than the percentages beside them.
# gcovr rounds a percentage to one decimal.
# So 17373 of 17375 branches read as 100.0 and passed this gate.
# That is about a dozen uncovered branches at this tree's size.
# A count is exact, and one branch short is what the gate is for.
import argparse
import json
import sys
from pathlib import Path

# The three things measured, and how a report names each.
KINDS = (
    ("line", "Lines"),
    ("function", "Functions"),
    ("branch", "Branches"),
)


def counts_of(summary: dict, kind: str) -> tuple:
    return (summary[f"{kind}_covered"], summary[f"{kind}_total"])


def shortfalls(summary: dict) -> list:
    missed = []

    for kind, label in KINDS:
        covered, total = counts_of(summary, kind)

        if covered != total:
            missed.append((label, total - covered))

    return missed


def is_full_coverage(summary: dict) -> bool:
    return not shortfalls(summary)


def report(summary: dict) -> str:
    parts = []

    for kind, label in KINDS:
        covered, total = counts_of(summary, kind)
        parts.append(f"{label}: {covered}/{total}")

    return " ".join(parts)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--summary",
        type=Path,
        required=True,
        help="Path to a gcovr --json-summary report",
    )
    args = parser.parse_args()

    with args.summary.open() as f:
        summary = json.load(f)

    print(report(summary))

    missed = shortfalls(summary)

    if missed:
        for label, short in missed:
            print(f"{label}: {short} uncovered.")

        print(
            "Coverage must be 100% lines/functions/branches -- see "
            "docs/confirming-unreachable-branches.md."
        )
        return 1

    print("OK: coverage is 100% lines/functions/branches.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
