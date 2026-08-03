#!/usr/bin/env bash
#
# Reports coverage over an instrumented build, the way CI measures it.
#
# The gcovr invocation lived in three places -- the CI workflow, a VS
# Code task and two documents -- and only the workflow's carried the
# three exclusions the 100% gate is computed against.
# A developer following the documented command therefore saw a branch
# percentage well below the enforced one and could not tell whether a
# push would pass.
# This script is the one copy all four now call, so the number a local
# run prints is the number the gate reads.
#
# GCOV_EXECUTABLE names the gcov to use, which differs per toolchain:
# "gcov" under GNU and "llvm-cov-<n> gcov" under LLVM.

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

build_dir=build-coverage
summary=""
html_dir=""

usage() {
    echo "Usage: scripts/coverage.sh [options]" >&2
    echo "  --build-dir <dir>  Instrumented build (default:" \
        "build-coverage)" >&2
    echo "  --summary <file>   Also write a JSON summary there" >&2
    echo "  --html <dir>       Also write an HTML report there" >&2
}

# 'shift 2' with one argument left fails, and under 'set -e' that
# ends the script with no message at all.
# So an option missing its value is refused with the usage instead.
require_value() {
    if [ "$#" -lt 2 ]; then
        echo "Option '$1' needs a value." >&2
        usage
        exit 1
    fi
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --build-dir)
            require_value "$@"
            build_dir=$2
            shift 2
            ;;
        --summary)
            require_value "$@"
            summary=$2
            shift 2
            ;;
        --html)
            require_value "$@"
            html_dir=$2
            shift 2
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown argument '$1'." >&2
            usage
            exit 1
            ;;
    esac
done

if [ -z "$build_dir" ] || [ ! -d "$build_dir" ]; then
    echo "No instrumented build at '$build_dir'." >&2
    echo "Run 'cmake --preset conan-coverage' and build it first." >&2
    exit 1
fi

# Every app's main.cpp is the one file left out of the report, and
# docs/STYLE_GUIDE.md says what it has to hold to earn that.
# The two branch exclusions drop compiler-generated exception-unwind
# edges and branches the compiler proved unreachable, neither of which
# is this project's own logic.
gcovr_args=(
    --root .
    --filter 'src/.*'
    --exclude '.*/tests/.*'
    --exclude '.*/apps/[^/]+/src/main\.cpp'
    --exclude-throw-branches
    --exclude-unreachable-branches
    --gcov-executable "${GCOV_EXECUTABLE:-gcov}"
    --print-summary
)

# Each output format is given its own filename rather than sharing one
# -o, which is what lets a single run write both.
if [ -n "$summary" ]; then
    gcovr_args+=(--json-summary-pretty --json-summary "$summary")
fi

if [ -n "$html_dir" ]; then
    rm -Rf "$html_dir"
    mkdir -p "$html_dir"
    gcovr_args+=(--html-details "$html_dir/index.html")
fi

gcovr "${gcovr_args[@]}" "$build_dir"
