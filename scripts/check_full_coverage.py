#!/usr/bin/env python3
# Fails unless a gcovr --json-summary report is 100% covered.
# Checks lines, functions, and branches, each independently.
import argparse
import json
import sys
from pathlib import Path


def is_full_coverage(summary: dict) -> bool:
    return (
        summary["line_percent"] == 100
        and summary["function_percent"] == 100
        and summary["branch_percent"] == 100
    )


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

    lines = summary["line_percent"]
    functions = summary["function_percent"]
    branches = summary["branch_percent"]

    print(
        f"Lines: {lines:.1f}% Functions: {functions:.1f}% "
        f"Branches: {branches:.1f}%"
    )

    if not is_full_coverage(summary):
        print(
            "Coverage must be 100% lines/functions/branches -- see "
            "docs/confirming-unreachable-branches.md."
        )
        return 1

    print("OK: coverage is 100% lines/functions/branches.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
