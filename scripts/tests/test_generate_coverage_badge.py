#!/usr/bin/env python3
# Plain-assert tests for generate_coverage_badge.py. Run directly:
#   python3 scripts/tests/test_generate_coverage_badge.py
import importlib.util
import json
import sys
import tempfile
from pathlib import Path

SCRIPT_PATH = Path(__file__).resolve().parent.parent / "generate_coverage_badge.py"

spec = importlib.util.spec_from_file_location("generate_coverage_badge", SCRIPT_PATH)
generate_coverage_badge = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generate_coverage_badge)


def run_main(summary, label="coverage (gnu)"):
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        summary_path = tmp_path / "coverage-summary.json"
        output_path = tmp_path / "coverage-badge.json"
        summary_path.write_text(json.dumps(summary))

        old_argv = sys.argv
        sys.argv = [
            "generate_coverage_badge.py",
            "--summary", str(summary_path),
            "--output", str(output_path),
            "--label", label,
        ]
        try:
            generate_coverage_badge.main()
        finally:
            sys.argv = old_argv

        return json.loads(output_path.read_text())


def it_picks_color_from_gcovr_thresholds():
    assert generate_coverage_badge.color_for(100.0) == generate_coverage_badge.HIGH_COLOR
    assert generate_coverage_badge.color_for(90.0) == generate_coverage_badge.HIGH_COLOR
    assert generate_coverage_badge.color_for(89.9) == generate_coverage_badge.MEDIUM_COLOR
    assert generate_coverage_badge.color_for(75.0) == generate_coverage_badge.MEDIUM_COLOR
    assert generate_coverage_badge.color_for(74.9) == generate_coverage_badge.LOW_COLOR
    assert generate_coverage_badge.color_for(0.0) == generate_coverage_badge.LOW_COLOR


def it_writes_a_full_coverage_badge():
    badge = run_main({
        "line_percent": 100.0,
        "function_percent": 100.0,
        "branch_percent": 100.0,
    })

    assert badge == {
        "schemaVersion": 1,
        "label": "coverage (gnu)",
        "message": "L:100.0% F:100.0% B:100.0%",
        "color": generate_coverage_badge.HIGH_COLOR,
    }


def it_uses_the_lowest_metric_for_color():
    badge = run_main({
        "line_percent": 100.0,
        "function_percent": 95.0,
        "branch_percent": 60.0,
    })

    assert badge["color"] == generate_coverage_badge.LOW_COLOR
    assert badge["message"] == "L:100.0% F:95.0% B:60.0%"


def it_writes_the_provided_label():
    badge = run_main({
        "line_percent": 100.0,
        "function_percent": 100.0,
        "branch_percent": 100.0,
    }, label="coverage (llvm)")

    assert badge["label"] == "coverage (llvm)"


def main():
    tests = [
        it_picks_color_from_gcovr_thresholds,
        it_writes_a_full_coverage_badge,
        it_uses_the_lowest_metric_for_color,
        it_writes_the_provided_label,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
