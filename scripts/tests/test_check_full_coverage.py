#!/usr/bin/env python3
# Plain-assert tests for check_full_coverage.py.
# Run directly:
#   python3 scripts/tests/test_check_full_coverage.py
import importlib.util
import io
import json
import sys
import tempfile
from contextlib import redirect_stdout
from pathlib import Path

SCRIPT_PATH = (
    Path(__file__).resolve().parent.parent / "check_full_coverage.py"
)

spec = importlib.util.spec_from_file_location(
    "check_full_coverage", SCRIPT_PATH
)
check_full_coverage = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check_full_coverage)


def run_main(lines: tuple, functions: tuple, branches: tuple):
    with tempfile.TemporaryDirectory() as tmp:
        summary_path = Path(tmp) / "coverage-summary.json"

        # The percentages are written too, as gcovr writes them.
        # Nothing reads them, which is the point of these tests.
        summary_path.write_text(
            json.dumps(
                {
                    "line_covered": lines[0],
                    "line_total": lines[1],
                    "line_percent": 100.0,
                    "function_covered": functions[0],
                    "function_total": functions[1],
                    "function_percent": 100.0,
                    "branch_covered": branches[0],
                    "branch_total": branches[1],
                    "branch_percent": 100.0,
                }
            )
        )

        old_argv = sys.argv
        sys.argv = [
            "check_full_coverage.py",
            "--summary", str(summary_path),
        ]
        stdout = io.StringIO()
        try:
            with redirect_stdout(stdout):
                exit_code = check_full_coverage.main()
        finally:
            sys.argv = old_argv

        return exit_code, stdout.getvalue()


def it_passes_at_exactly_full_coverage():
    exit_code, stdout = run_main((10, 10), (4, 4), (6, 6))

    assert exit_code == 0
    assert "OK: coverage is 100% lines/functions/branches." in stdout


def it_fails_when_lines_are_below_full_coverage():
    exit_code, stdout = run_main((9, 10), (4, 4), (6, 6))

    assert exit_code == 1
    assert "Lines: 9/10" in stdout
    assert "Lines: 1 uncovered." in stdout


def it_fails_when_functions_are_below_full_coverage():
    exit_code, stdout = run_main((10, 10), (3, 4), (6, 6))

    assert exit_code == 1
    assert "Functions: 3/4" in stdout
    assert "Functions: 1 uncovered." in stdout


def it_fails_when_branches_are_below_full_coverage():
    exit_code, stdout = run_main((10, 10), (4, 4), (5, 6))

    assert exit_code == 1
    assert "Branches: 5/6" in stdout
    assert "Branches: 1 uncovered." in stdout


# The hole this gate had: a shortfall too small to round away.
# 17373 of 17375 branches is 99.9885%, which gcovr writes as 100.0.
# Reading that field passed a tree with two branches nothing took.
def it_fails_when_a_shortfall_rounds_up_to_full():
    exit_code, stdout = run_main((10, 10), (4, 4), (17373, 17375))

    assert exit_code == 1
    assert "Branches: 17373/17375" in stdout
    assert "Branches: 2 uncovered." in stdout


# Every shortfall is named, not just the first one found.
def it_names_every_kind_that_fell_short():
    exit_code, stdout = run_main((9, 10), (3, 4), (5, 6))

    assert exit_code == 1
    assert "Lines: 1 uncovered." in stdout
    assert "Functions: 1 uncovered." in stdout
    assert "Branches: 1 uncovered." in stdout


# A report measuring nothing has nothing uncovered in it.
# gcovr writes zeroes rather than omitting the members.
def it_passes_when_nothing_was_measured():
    exit_code, stdout = run_main((0, 0), (0, 0), (0, 0))

    assert exit_code == 0
    assert "OK: coverage is 100% lines/functions/branches." in stdout


def main():
    tests = [
        it_passes_at_exactly_full_coverage,
        it_fails_when_lines_are_below_full_coverage,
        it_fails_when_functions_are_below_full_coverage,
        it_fails_when_branches_are_below_full_coverage,
        it_fails_when_a_shortfall_rounds_up_to_full,
        it_names_every_kind_that_fell_short,
        it_passes_when_nothing_was_measured,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
