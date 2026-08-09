#!/usr/bin/env python3

import argparse
import json
import sys
from pathlib import Path

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
    parser = argparse.ArgumentParser()
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

        print("Coverage must be 100% lines, functions and branches.")
        return 1

    print("OK: coverage is 100% lines/functions/branches.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
