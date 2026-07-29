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


def run_main(lines: float, functions: float, branches: float):
    with tempfile.TemporaryDirectory() as tmp:
        summary_path = Path(tmp) / "coverage-summary.json"
        summary_path.write_text(
            json.dumps(
                {
                    "line_percent": lines,
                    "function_percent": functions,
                    "branch_percent": branches,
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
    exit_code, stdout = run_main(100.0, 100.0, 100.0)

    assert exit_code == 0
    assert "OK: coverage is 100% lines/functions/branches." in stdout


def it_fails_when_lines_are_below_full_coverage():
    exit_code, stdout = run_main(99.5, 100.0, 100.0)

    assert exit_code == 1
    assert "Lines: 99.5%" in stdout


def it_fails_when_functions_are_below_full_coverage():
    exit_code, stdout = run_main(100.0, 95.0, 100.0)

    assert exit_code == 1
    assert "Functions: 95.0%" in stdout


def it_fails_when_branches_are_below_full_coverage():
    exit_code, stdout = run_main(100.0, 100.0, 99.0)

    assert exit_code == 1
    assert "Branches: 99.0%" in stdout


def main():
    tests = [
        it_passes_at_exactly_full_coverage,
        it_fails_when_lines_are_below_full_coverage,
        it_fails_when_functions_are_below_full_coverage,
        it_fails_when_branches_are_below_full_coverage,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
