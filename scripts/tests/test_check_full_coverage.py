#!/usr/bin/env python3

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


def summary_of(lines: tuple, functions: tuple, branches: tuple) -> str:
    return json.dumps(
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


def run_main(
    lines: tuple,
    functions: tuple,
    branches: tuple,
    baseline: tuple | None = None,
) -> int:
    with tempfile.TemporaryDirectory() as tmp:
        summary_path = Path(tmp) / "coverage-summary.json"

        summary_path.write_text(summary_of(lines, functions, branches))

        old_argv = sys.argv
        sys.argv = [
            "check_full_coverage.py",
            "--summary", str(summary_path),
        ]

        if baseline is not None:
            baseline_path = Path(tmp) / "coverage-baseline.json"
            baseline_path.write_text(summary_of(*baseline))
            sys.argv += ["--baseline", str(baseline_path)]
        stdout = io.StringIO()
        try:
            with redirect_stdout(stdout):
                exit_code = check_full_coverage.main()
        finally:
            sys.argv = old_argv

        return exit_code, stdout.getvalue()


def it_passes_at_exactly_full_coverage() -> None:
    exit_code, stdout = run_main((10, 10), (4, 4), (6, 6))

    assert exit_code == 0
    assert "OK: coverage is 100% lines/functions/branches." in stdout


def it_fails_when_lines_are_below_full_coverage() -> None:
    exit_code, stdout = run_main((9, 10), (4, 4), (6, 6))

    assert exit_code == 1
    assert "Lines: 9/10" in stdout
    assert "Lines: 1 uncovered." in stdout


def it_fails_when_functions_are_below_full_coverage() -> None:
    exit_code, stdout = run_main((10, 10), (3, 4), (6, 6))

    assert exit_code == 1
    assert "Functions: 3/4" in stdout
    assert "Functions: 1 uncovered." in stdout


def it_fails_when_branches_are_below_full_coverage() -> None:
    exit_code, stdout = run_main((10, 10), (4, 4), (5, 6))

    assert exit_code == 1
    assert "Branches: 5/6" in stdout
    assert "Branches: 1 uncovered." in stdout


def it_fails_when_a_shortfall_rounds_up_to_full() -> None:
    exit_code, stdout = run_main((10, 10), (4, 4), (17373, 17375))

    assert exit_code == 1
    assert "Branches: 17373/17375" in stdout
    assert "Branches: 2 uncovered." in stdout


def it_names_every_kind_that_fell_short() -> None:
    exit_code, stdout = run_main((9, 10), (3, 4), (5, 6))

    assert exit_code == 1
    assert "Lines: 1 uncovered." in stdout
    assert "Functions: 1 uncovered." in stdout
    assert "Branches: 1 uncovered." in stdout


def it_passes_when_nothing_was_measured() -> None:
    exit_code, stdout = run_main((0, 0), (0, 0), (0, 0))

    assert exit_code == 0
    assert "OK: coverage is 100% lines/functions/branches." in stdout


def it_passes_when_a_baseline_is_held() -> None:
    exit_code, stdout = run_main(
        (9, 10),
        (3, 4),
        (5, 6),
        baseline=((9, 10), (3, 4), (5, 6)),
    )

    assert exit_code == 0
    assert "OK: coverage held, though it is not yet 100%." in stdout


def it_passes_when_a_baseline_is_bettered() -> None:
    exit_code, stdout = run_main(
        (10, 10),
        (4, 4),
        (6, 6),
        baseline=((9, 10), (3, 4), (5, 6)),
    )

    assert exit_code == 0
    assert "OK: coverage is 100% lines/functions/branches." in stdout


def it_fails_when_a_share_falls_below_its_baseline() -> None:
    exit_code, stdout = run_main(
        (8, 10),
        (3, 4),
        (5, 6),
        baseline=((9, 10), (3, 4), (5, 6)),
    )

    assert exit_code == 1
    assert "Lines: 80.00% covers less than 90.00%." in stdout
    assert "Coverage may not fall." in stdout


def it_holds_a_share_kept_while_the_code_grew() -> None:
    exit_code, stdout = run_main(
        (18, 20),
        (3, 4),
        (5, 6),
        baseline=((9, 10), (3, 4), (5, 6)),
    )

    assert exit_code == 0
    assert "OK: coverage held, though it is not yet 100%." in stdout


def it_ignores_a_kind_nothing_was_measured_for() -> None:
    exit_code, stdout = run_main(
        (9, 10),
        (0, 0),
        (5, 6),
        baseline=((9, 10), (3, 4), (5, 6)),
    )

    assert exit_code == 0
    assert "OK: coverage held, though it is not yet 100%." in stdout


def main() -> None:
    tests = [
        it_passes_at_exactly_full_coverage,
        it_fails_when_lines_are_below_full_coverage,
        it_fails_when_functions_are_below_full_coverage,
        it_fails_when_branches_are_below_full_coverage,
        it_fails_when_a_shortfall_rounds_up_to_full,
        it_names_every_kind_that_fell_short,
        it_passes_when_nothing_was_measured,
        it_passes_when_a_baseline_is_held,
        it_passes_when_a_baseline_is_bettered,
        it_fails_when_a_share_falls_below_its_baseline,
        it_holds_a_share_kept_while_the_code_grew,
        it_ignores_a_kind_nothing_was_measured_for,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
