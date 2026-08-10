#!/usr/bin/env python3

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HEADER_SUFFIXES = (".hpp", ".h", ".inl")

def tree_filters(root: Path) -> list[str]:
    stem = str(root).rstrip("/")

    return [
        "--filter", f"{stem}/src/.*",
        "--exclude", ".*/tests/.*",
        "--exclude", r".*/apps/[^/]+/src/main\.cpp",
    ]


def repo_root() -> Path:
    named = os.environ.get("ANTWIKA_ROOT")

    if named:
        return Path(named).resolve()

    here = Path(__file__).resolve().parent
    found = subprocess.run(
        ["git", "-C", str(here), "rev-parse", "--show-toplevel"],
        capture_output=True,
        text=True,
    )

    if found.returncode != 0:
        raise SystemExit("Not inside a git checkout, and ANTWIKA_ROOT is unset.")

    return Path(found.stdout.strip())


def object_dirs(root: Path, build: Path, source: str) -> list[Path]:
    base = Path(source).name
    found = {
        path.parent
        for path in build.rglob(base + ".gcda")
    }

    if found:
        return sorted(found)

    if base.endswith(HEADER_SUFFIXES):
        return sorted(
            path for path in build.rglob("*.dir") if path.is_dir()
        )

    return []


def run_gcovr(root: Path, search: list[Path], scope: list[str]) -> dict:
    work = tempfile.mkdtemp()

    try:
        report = Path(work) / "report.json"
        command = [
            "gcovr",
            "--root", str(root),
            *scope,
            "--exclude-throw-branches",
            "--exclude-unreachable-branches",
            "--gcov-executable", os.environ.get("GCOV_EXECUTABLE", "gcov"),
            "--json", str(report),
            *[str(path) for path in search],
        ]
        finished = subprocess.run(
            command, cwd=work, capture_output=True, text=True
        )

        if not report.exists():
            sys.stderr.write(finished.stderr[-2000:])
            raise SystemExit("gcovr wrote no report.")

        with report.open() as stream:
            return json.load(stream)
    finally:
        shutil.rmtree(work, ignore_errors=True)


def gaps_of(entry: dict) -> tuple[list[int], list[tuple[int, int]], int, int]:
    bare = []
    partial = []
    branches = 0
    taken = 0

    for line in entry.get("lines", []):
        if line.get("gcovr/excluded", False) or line.get(
            "gcovr/noncode", False
        ):
            continue

        number = line["line_number"]

        if line.get("count", 0) == 0:
            bare.append(number)

        here = [
            branch
            for branch in line.get("branches", [])
            if not branch.get("gcovr/excluded", False)
        ]
        hit = sum(1 for branch in here if branch.get("count", 0) > 0)
        branches += len(here)
        taken += hit

        if here and hit < len(here):
            partial.append((number, len(here) - hit))

    return bare, partial, taken, branches


def functions_of(entry: dict) -> tuple[int, int]:
    named = [
        one
        for one in entry.get("functions", [])
        if not one.get("gcovr/excluded", False)
    ]
    called = sum(1 for one in named if one.get("execution_count", 0) > 0)

    return called, len(named)


def report(data: dict, root: Path, quiet: bool) -> int:
    clean = True

    for entry in sorted(data.get("files", []), key=lambda one: one["file"]):
        name = entry["file"]
        bare, partial, taken, branches = gaps_of(entry)
        called, total = functions_of(entry)

        if not bare and not partial and called == total:
            if not quiet:
                print(f"{name}: covered")
            continue

        clean = False
        print(f"{name}")

        if called != total:
            print(f"  functions: {called}/{total} called")

        if bare:
            print(f"  lines never executed: {squash(bare)}")

        if partial:
            shown = ", ".join(
                f"{line} ({missed})" for line, missed in partial
            )
            print(f"  branches not taken, by line: {shown}")

        print(f"  branch total: {taken}/{branches}")

    return 0 if clean else 2


def squash(numbers: list[int]) -> str:
    runs = []

    for number in numbers:
        if runs and number == runs[-1][1] + 1:
            runs[-1][1] = number
        else:
            runs.append([number, number])

    return ", ".join(
        str(low) if low == high else f"{low}-{high}" for low, high in runs
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="List uncovered lines and branches, whole tree or per file."
    )
    parser.add_argument(
        "sources",
        nargs="*",
        help="Repo-relative source paths. With none, sweeps the whole tree.",
    )
    parser.add_argument(
        "--build-dir",
        default=os.environ.get("ANTWIKA_COVERAGE_BUILD", "build-coverage"),
        help="Instrumented build (default: build-coverage)",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="Print only the files that have gaps",
    )
    args = parser.parse_args()

    root = repo_root()
    build = root / args.build_dir

    if not build.is_dir():
        raise SystemExit(f"No instrumented build at '{build}'.")

    if not args.sources:
        data = run_gcovr(root, [build], tree_filters(root))
        seen = len(data.get("files", []))

        if seen == 0:
            raise SystemExit(
                "gcovr matched no source files at all, which is a broken"
                " invocation and not a clean tree. Check the filters and"
                f" that '{build}' holds .gcda files."
            )

        outcome = report(data, root, quiet=True)

        if outcome == 0:
            print(f"No gaps: {seen} measured files are fully covered.")

        return outcome

    outcome = 0

    for source in args.sources:
        if not (root / source).exists():
            print(f"{source}: no such file", file=sys.stderr)
            outcome = max(outcome, 1)
            continue

        search = object_dirs(root, build, source)

        if not search:
            print(
                f"{source}: no .gcda under {args.build_dir}"
                " — is it compiled into anything?",
                file=sys.stderr,
            )
            outcome = max(outcome, 1)
            continue

        data = run_gcovr(
            root, search, ["--filter", str(root / source)]
        )

        if not data.get("files"):
            print(f"{source}: gcovr matched no coverage data", file=sys.stderr)
            outcome = max(outcome, 1)
            continue

        outcome = max(outcome, report(data, root, quiet=args.quiet))

    return outcome


if __name__ == "__main__":
    sys.exit(main())
