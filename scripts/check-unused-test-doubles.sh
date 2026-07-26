#!/usr/bin/env bash
# Fails if any mock/fake header under src/**/tests/{mocks,fakes}/include is
# never #included by a .cpp file, i.e. it's dead test-double code.
set -euo pipefail

readonly repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

find_test_doubles() {
    find src -type f -name "*.hpp" \
        \( -path "*/tests/mocks/include/*" -o -path "*/tests/fakes/include/*" \) \
        | sort
}

is_included_anywhere() {
    local header_name="$1"
    grep -rlF --include="*.cpp" -- "$header_name" src >/dev/null 2>&1
}

report_orphans() {
    local -n orphan_list="$1"

    echo "The following test doubles are never included by any .cpp file:"
    for orphan in "${orphan_list[@]}"; do
        echo "  - $orphan"
    done
    echo
    echo "Either delete the unused test double or add a test that uses it."
}

main() {
    mapfile -t doubles < <(find_test_doubles)

    if [ "${#doubles[@]}" -eq 0 ]; then
        echo "No test doubles found under src/**/tests/{mocks,fakes}/include -- did the layout change?" >&2
        exit 1
    fi

    local orphans=()
    for header in "${doubles[@]}"; do
        is_included_anywhere "$(basename "$header")" || orphans+=("$header")
    done

    if [ "${#orphans[@]}" -gt 0 ]; then
        report_orphans orphans
        exit 1
    fi

    echo "OK: every mock/fake header is included by at least one .cpp file (${#doubles[@]} checked)."
}

main
