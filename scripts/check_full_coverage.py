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


def slipped(summary: dict, baseline: dict) -> list:
    lost = []

    for kind, label in KINDS:
        covered, total = counts_of(summary, kind)
        was_covered, was_total = counts_of(baseline, kind)

        if total == 0 or was_total == 0:
            continue

        if covered * was_total < was_covered * total:
            lost.append(
                (
                    label,
                    100.0 * covered / total,
                    100.0 * was_covered / was_total,
                )
            )

    return lost


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
    parser.add_argument(
        "--baseline",
        type=Path,
        help=(
            "Path to a summary the report may not cover a smaller "
            "share than. Without it, coverage must be 100%%."
        ),
    )
    args = parser.parse_args()

    with args.summary.open() as f:
        summary = json.load(f)

    print(report(summary))

    missed = shortfalls(summary)

    if not missed:
        print("OK: coverage is 100% lines/functions/branches.")
        return 0

    for label, short in missed:
        print(f"{label}: {short} uncovered.")

    if args.baseline is None:
        print("Coverage must be 100% lines, functions and branches.")
        return 1

    with args.baseline.open() as f:
        baseline = json.load(f)

    lost = slipped(summary, baseline)

    if lost:
        for label, now, was in lost:
            print(f"{label}: {now:.2f}% covers less than {was:.2f}%.")

        print(
            "Coverage may not fall. Cover what was added, or say why "
            "in the commit that moves the baseline."
        )
        return 1

    print("OK: coverage held, though it is not yet 100%.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
