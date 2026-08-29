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

sys.path.insert(0, str(SCRIPT_PATH.parent))

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


def totals_of(lines: tuple, functions: tuple, branches: tuple) -> dict:
    return {
        "line_covered": lines[0],
        "line_total": lines[1],
        "function_covered": functions[0],
        "function_total": functions[1],
        "branch_covered": branches[0],
        "branch_total": branches[1],
    }


def file_entry(
    filename: str,
    lines: tuple,
    functions: tuple,
    branches: tuple,
) -> dict:
    return {"filename": filename, **totals_of(lines, functions, branches)}


def run_check(
    summary: dict,
    baseline: dict | None = None,
    write_baseline: Path | None = None,
) -> tuple:
    with tempfile.TemporaryDirectory() as tmp:
        summary_path = Path(tmp) / "coverage-summary.json"
        summary_path.write_text(json.dumps(summary))

        old_argv = sys.argv
        sys.argv = [
            "check_full_coverage.py",
            "--summary", str(summary_path),
        ]

        if baseline is not None:
            baseline_path = Path(tmp) / "coverage-baseline.json"
            baseline_path.write_text(json.dumps(baseline))
            sys.argv += ["--baseline", str(baseline_path)]

        if write_baseline is not None:
            sys.argv += ["--write-baseline", str(write_baseline)]

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


def it_groups_files_into_libraries() -> None:
    summary = {
        "files": [
            file_entry("src/libs/map/src/A.cpp", (4, 5), (1, 1), (2, 3)),
            file_entry("src/libs/map/src/B.cpp", (1, 5), (1, 2), (1, 3)),
            file_entry("src/apps/editor/src/C.cpp", (2, 2), (1, 1), (0, 0)),
            file_entry("backends/raylib/src/D.cpp", (3, 4), (2, 2), (1, 2)),
            file_entry("cmake/E.cpp", (9, 9), (9, 9), (9, 9)),
        ],
    }

    libraries = check_full_coverage.libraries_of(summary)

    assert sorted(libraries) == [
        "backends/raylib", "src/apps/editor", "src/libs/map",
    ]
    assert libraries["src/libs/map"] == totals_of((5, 10), (2, 3), (3, 6))


def it_falls_back_to_global_when_the_baseline_has_no_libraries() -> None:
    summary = {
        **totals_of((9, 10), (3, 4), (5, 6)),
        "files": [
            file_entry("src/libs/a/src/A.cpp", (0, 5), (0, 2), (0, 3)),
            file_entry("src/libs/b/src/B.cpp", (9, 5), (3, 2), (5, 3)),
        ],
    }
    baseline = totals_of((9, 10), (3, 4), (5, 6))

    exit_code, stdout = run_check(summary, baseline)

    assert exit_code == 0
    assert "no per-library section" in stdout
    assert "OK: coverage held, though it is not yet 100%." in stdout


def it_fails_when_a_library_falls_below_its_baseline() -> None:
    summary = {
        **totals_of((9, 10), (4, 4), (6, 6)),
        "files": [
            file_entry("src/libs/a/src/A.cpp", (4, 5), (2, 2), (3, 3)),
            file_entry("src/libs/b/src/B.cpp", (5, 5), (2, 2), (3, 3)),
        ],
    }
    baseline = {
        **totals_of((9, 10), (4, 4), (6, 6)),
        "libraries": {
            "src/libs/a": totals_of((5, 5), (2, 2), (3, 3)),
            "src/libs/b": totals_of((4, 5), (2, 2), (3, 3)),
        },
    }

    exit_code, stdout = run_check(summary, baseline)

    assert exit_code == 1
    assert "src/libs/a Lines: 80.00% covers less than 100.00%." in stdout
    assert "Coverage may not fall." in stdout


def it_passes_when_every_library_held_its_share() -> None:
    summary = {
        **totals_of((9, 10), (3, 4), (5, 6)),
        "files": [
            file_entry("src/libs/a/src/A.cpp", (4, 5), (1, 2), (2, 3)),
            file_entry("src/libs/b/src/B.cpp", (5, 5), (2, 2), (3, 3)),
        ],
    }
    baseline = {
        **totals_of((9, 10), (3, 4), (5, 6)),
        "libraries": {
            "src/libs/a": totals_of((4, 5), (1, 2), (2, 3)),
            "src/libs/b": totals_of((5, 5), (2, 2), (3, 3)),
        },
    }

    exit_code, stdout = run_check(summary, baseline)

    assert exit_code == 0
    assert "OK: coverage held, though it is not yet 100%." in stdout


def it_ignores_a_library_missing_from_the_baseline() -> None:
    summary = {
        **totals_of((9, 10), (3, 4), (5, 6)),
        "files": [
            file_entry("src/libs/new/src/A.cpp", (9, 10), (3, 4), (5, 6)),
        ],
    }
    baseline = {
        **totals_of((9, 10), (3, 4), (5, 6)),
        "libraries": {},
    }

    exit_code, stdout = run_check(summary, baseline)

    assert exit_code == 0
    assert "OK: coverage held, though it is not yet 100%." in stdout


def it_ignores_a_library_gone_from_the_summary() -> None:
    summary = {
        **totals_of((9, 10), (3, 4), (5, 6)),
        "files": [
            file_entry("src/libs/a/src/A.cpp", (9, 10), (3, 4), (5, 6)),
        ],
    }
    baseline = {
        **totals_of((9, 10), (3, 4), (5, 6)),
        "libraries": {
            "src/libs/gone": totals_of((5, 5), (2, 2), (3, 3)),
            "src/libs/a": totals_of((9, 10), (3, 4), (5, 6)),
        },
    }

    exit_code, stdout = run_check(summary, baseline)

    assert exit_code == 0
    assert "OK: coverage held, though it is not yet 100%." in stdout


def it_prefers_an_embedded_library_section_in_the_summary() -> None:
    summary = {
        **totals_of((9, 10), (3, 4), (5, 6)),
        "libraries": {
            "src/libs/a": totals_of((4, 5), (1, 2), (2, 3)),
        },
    }
    baseline = {
        **totals_of((9, 10), (3, 4), (5, 6)),
        "libraries": {
            "src/libs/a": totals_of((5, 5), (2, 2), (3, 3)),
        },
    }

    exit_code, stdout = run_check(summary, baseline)

    assert exit_code == 1
    assert "src/libs/a Lines: 80.00% covers less than 100.00%." in stdout


def it_notices_a_summary_without_per_file_data() -> None:
    summary = totals_of((9, 10), (3, 4), (5, 6))
    baseline = {
        **totals_of((9, 10), (3, 4), (5, 6)),
        "libraries": {
            "src/libs/a": totals_of((5, 5), (2, 2), (3, 3)),
        },
    }

    exit_code, stdout = run_check(summary, baseline)

    assert exit_code == 0
    assert "no per-file data" in stdout
    assert "OK: coverage held, though it is not yet 100%." in stdout


def it_writes_a_baseline_with_a_library_section() -> None:
    summary = {
        **totals_of((9, 10), (3, 4), (5, 6)),
        "files": [
            file_entry("src/libs/a/src/A.cpp", (4, 5), (1, 2), (2, 3)),
            file_entry("src/libs/a/src/B.cpp", (5, 5), (2, 2), (3, 3)),
        ],
    }

    with tempfile.TemporaryDirectory() as tmp:
        out_path = Path(tmp) / "coverage-baseline.json"

        exit_code, stdout = run_check(summary, write_baseline=out_path)
        written = json.loads(out_path.read_text())

    assert exit_code == 0
    assert f"Wrote a per-library baseline to {out_path}." in stdout
    assert written == {
        **totals_of((9, 10), (3, 4), (5, 6)),
        "libraries": {
            "src/libs/a": totals_of((9, 10), (3, 4), (5, 6)),
        },
    }


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
        it_groups_files_into_libraries,
        it_falls_back_to_global_when_the_baseline_has_no_libraries,
        it_fails_when_a_library_falls_below_its_baseline,
        it_passes_when_every_library_held_its_share,
        it_ignores_a_library_missing_from_the_baseline,
        it_ignores_a_library_gone_from_the_summary,
        it_prefers_an_embedded_library_section_in_the_summary,
        it_notices_a_summary_without_per_file_data,
        it_writes_a_baseline_with_a_library_section,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
