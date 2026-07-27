#!/usr/bin/env python3
# Turns a gcovr --json-summary report into a shields.io endpoint badge,
# using gcovr's own low/medium/high buckets and colors (see index.css: theme-green).
import argparse
import json

LOW_COLOR = "FF6666"
MEDIUM_COLOR = "F9FD63"
HIGH_COLOR = "85E485"


def color_for(percent: float) -> str:
    if percent >= 90:
        return HIGH_COLOR
    if percent >= 75:
        return MEDIUM_COLOR
    return LOW_COLOR


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--summary", required=True, help="Path to a gcovr --json-summary report")
    parser.add_argument("--output", required=True, help="Path to write the shields.io endpoint badge JSON")
    parser.add_argument("--label", required=True, help="Badge label, e.g. 'coverage (gnu)'")
    args = parser.parse_args()

    with open(args.summary) as f:
        summary = json.load(f)

    lines = summary["line_percent"]
    functions = summary["function_percent"]
    branches = summary["branch_percent"]
    color = color_for(min(lines, functions, branches))

    badge = {
        "schemaVersion": 1,
        "label": args.label,
        "message": f"L:{lines:.1f}% F:{functions:.1f}% B:{branches:.1f}%",
        "color": color,
    }

    with open(args.output, "w") as f:
        json.dump(badge, f, indent=2)

    print(f"Lines: {lines:.1f}% Functions: {functions:.1f}% Branches: {branches:.1f}% ({color})")


if __name__ == "__main__":
    main()
