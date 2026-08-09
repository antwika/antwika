#!/usr/bin/env bash

set -uo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/../../../.."

build_dir=build
repeat=3

usage() {
    echo "Usage: check_isolation.sh [--build-dir <dir>] [--repeat <n>]" >&2
    echo "  Runs every test binary with its tests shuffled into one" >&2
    echo "  process, repeated, so an order dependence shows up." >&2
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --build-dir) build_dir=$2; shift 2 ;;
        --repeat) repeat=$2; shift 2 ;;
        -h | --help) usage; exit 0 ;;
        *) echo "Unknown argument '$1'." >&2; usage; exit 1 ;;
    esac
done

if [ ! -d "$build_dir" ]; then
    echo "No build at '$build_dir'." >&2
    exit 1
fi

checked=0
failed=0

for binary in $(find "$build_dir/bin" -maxdepth 2 -type f \
        -name "antwika_*_tests" | sort); do
    checked=$((checked + 1))

    output=$(SDL_AUDIO_DRIVER=dummy xvfb-run -a "$binary" \
        --gtest_shuffle --gtest_repeat="$repeat" --gtest_brief=1 2>&1)

    if echo "$output" | grep -q "FAILED"; then
        failed=$((failed + 1))
        echo "ORDER-DEPENDENT: $(basename "$binary")"
        echo "$output" | grep -E "^\[  FAILED  \] [A-Za-z]" | sort -u \
            | sed 's/^/    /'
    fi
done

echo
echo "binaries checked: $checked, order-dependent: $failed"
[ "$failed" -eq 0 ]
