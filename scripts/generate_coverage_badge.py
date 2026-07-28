#!/usr/bin/env python3
# Turns a gcovr --json-summary report into a shields.io endpoint badge.
# It uses gcovr's own low/medium/high buckets.
# It's colored with shields.io's standard palette.
# That matches the other badges in the README.
import argparse
import json

LOW_COLOR = "e05d44"
MEDIUM_COLOR = "dfb317"
HIGH_COLOR = "44cc11"


def color_for(percent: float) -> str:
    if percent >= 90:
        return HIGH_COLOR
    if percent >= 75:
        return MEDIUM_COLOR
    return LOW_COLOR


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--summary", required=True, help="Path to a gcovr --json-summary report"
    )
    parser.add_argument(
        "--output",
        required=True,
        help="Path to write the shields.io endpoint badge JSON",
    )
    parser.add_argument(
        "--label", required=True, help="Badge label, e.g. 'coverage (gnu)'"
    )
    args = parser.parse_args()

    with open(args.summary) as f:
        summary = json.load(f)

    lines = summary["line_percent"]
    functions = summary["function_percent"]
    branches = summary["branch_percent"]
    color = color_for(min(lines, functions, branches))

    if lines == 100 and functions == 100 and branches == 100:
        message = "100%"
    else:
        message = f"{round(lines)}%/{round(functions)}%/{round(branches)}%"

    badge = {
        "schemaVersion": 1,
        "label": args.label,
        "message": message,
        "color": color,
    }

    with open(args.output, "w") as f:
        json.dump(badge, f, indent=2)

    print(
        f"Lines: {lines:.1f}% Functions: {functions:.1f}% "
        f"Branches: {branches:.1f}% ({color})"
    )


if __name__ == "__main__":
    main()
