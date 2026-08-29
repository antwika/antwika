#!/usr/bin/env python3

import argparse
import json
import re
import sys
from pathlib import Path

KINDS = (
    ("line", "Lines"),
    ("function", "Functions"),
    ("branch", "Branches"),
)

LIBRARY = re.compile(r"^(src/libs/[^/]+|src/apps/[^/]+|backends/[^/]+)/")

NO_LIBRARY_BASELINE = (
    "Notice: the baseline has no per-library section, so only the "
    "global ratios are checked. Regenerate it with coverage.sh "
    "--baseline-out to ratchet each library."
)

NO_LIBRARY_SUMMARY = (
    "Notice: the summary has no per-file data, so only the global "
    "ratios are checked."
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


def library_of(filename: str) -> str | None:
    match = LIBRARY.match(filename)

    return match.group(1) if match else None


def aggregated_libraries(summary: dict) -> dict:
    libraries: dict = {}

    for entry in summary.get("files", []):
        name = library_of(entry["filename"])

        if name is None:
            continue

        into = libraries.setdefault(
            name,
            {
                f"{kind}_{part}": 0
                for kind, _ in KINDS
                for part in ("covered", "total")
            },
        )

        for kind, _ in KINDS:
            into[f"{kind}_covered"] += entry[f"{kind}_covered"]
            into[f"{kind}_total"] += entry[f"{kind}_total"]

    return libraries


def libraries_of(summary: dict) -> dict:
    embedded = summary.get("libraries")

    if embedded is not None:
        return embedded

    return aggregated_libraries(summary)


def slipped_libraries(summary: dict, baseline: dict) -> list:
    current = libraries_of(summary)
    lost = []

    for name, was in sorted(baseline["libraries"].items()):
        now = current.get(name)

        if now is None:
            continue

        for label, now_share, was_share in slipped(now, was):
            lost.append((name, label, now_share, was_share))

    return lost


def baseline_of(summary: dict) -> dict:
    slim: dict = {}

    for kind, _ in KINDS:
        covered, total = counts_of(summary, kind)
        slim[f"{kind}_covered"] = covered
        slim[f"{kind}_total"] = total

    slim["libraries"] = libraries_of(summary)

    return slim


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
            "share than, globally or per library. Without it, "
            "coverage must be 100%%."
        ),
    )
    parser.add_argument(
        "--write-baseline",
        type=Path,
        help=(
            "Write the summary as a ratchet baseline with a "
            "per-library section there, then exit without checking."
        ),
    )
    args = parser.parse_args()

    with args.summary.open() as f:
        summary = json.load(f)

    if args.write_baseline is not None:
        with args.write_baseline.open("w") as f:
            json.dump(baseline_of(summary), f, indent=2)
            f.write("\n")

        print(f"Wrote a per-library baseline to {args.write_baseline}.")
        return 0

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

    if "libraries" not in baseline:
        print(NO_LIBRARY_BASELINE)
        lost_libraries = []
    elif "libraries" not in summary and "files" not in summary:
        print(NO_LIBRARY_SUMMARY)
        lost_libraries = []
    else:
        lost_libraries = slipped_libraries(summary, baseline)

    if lost or lost_libraries:
        for label, now, was in lost:
            print(f"{label}: {now:.2f}% covers less than {was:.2f}%.")

        for name, label, now, was in lost_libraries:
            print(
                f"{name} {label}: {now:.2f}% covers less "
                f"than {was:.2f}%."
            )

        print(
            "Coverage may not fall. Cover what was added, or say why "
            "in the commit that moves the baseline."
        )
        return 1

    print("OK: coverage held, though it is not yet 100%.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
