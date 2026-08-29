#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

build_dir=build-coverage
summary=""
html_dir=""
baseline_out=""

usage() {
    echo "Usage: scripts/coverage.sh [options]" >&2
    echo "  --build-dir <dir>  Instrumented build (default:" \
        "build-coverage)" >&2
    echo "  --summary <file>   Also write a JSON summary there" >&2
    echo "  --html <dir>       Also write an HTML report there" >&2
    echo "  --baseline-out <file>  Also write a per-library ratchet" \
        "baseline there" >&2
}

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
        --baseline-out)
            require_value "$@"
            baseline_out=$2
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

if [ -n "$baseline_out" ] && [ -z "$summary" ]; then
    summary=$(mktemp)
    trap 'rm -f "$summary"' EXIT
fi

if [ -z "$build_dir" ] || [ ! -d "$build_dir" ]; then
    echo "No instrumented build at '$build_dir'." >&2
    echo "Run 'cmake --preset conan-coverage' and build it first." >&2
    exit 1
fi

gcovr_args=(
    --root .
    --filter 'src/.*'
    --filter 'backends/.*'
    --exclude '.*/tests/.*'
    --exclude '.*/apps/[^/]+/src/main\.cpp'
    --exclude-throw-branches
    --exclude-unreachable-branches
    --gcov-executable "${GCOV_EXECUTABLE:-gcov}"
    --print-summary
)

if [ -n "$summary" ]; then
    gcovr_args+=(--json-summary-pretty --json-summary "$summary")
fi

if [ -n "$html_dir" ]; then
    rm -Rf "$html_dir"
    mkdir -p "$html_dir"
    gcovr_args+=(--html-details "$html_dir/index.html")
fi

gcovr "${gcovr_args[@]}" "$build_dir" src backends

if [ -n "$baseline_out" ]; then
    python3 scripts/check_full_coverage.py \
        --summary "$summary" \
        --write-baseline "$baseline_out"
fi
