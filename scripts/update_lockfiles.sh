#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

source scripts/packages.sh

for editable in "${ANTWIKA_EDITABLES[@]}"; do
    conan editable add "$editable"
done

mapfile -t profiles < <(
    find profiles/host -mindepth 1 -maxdepth 1 -type f -printf '%f\n' \
        | sort
)

if [ "${#profiles[@]}" -eq 0 ]; then
    echo "No profiles found under profiles/host." >&2
    exit 1
fi

mapfile -t frameworks < <(
    conan inspect . -f json | python3 -c '
import json
import sys

definitions = json.load(sys.stdin)["options_definitions"]

for value in definitions["gfx_backend"]:
    if value != "null":
        print(value)
'
)

if [ "${#frameworks[@]}" -eq 0 ]; then
    echo "conanfile.py names no framework for gfx_backend." >&2
    exit 1
fi

for backend in null "${frameworks[@]}"; do
    options=()
    lockfile=conan.lock

    if [ "$backend" != "null" ]; then
        options=(-o "gfx_backend=$backend")
        lockfile="conan-${backend}.lock"
    fi

    base=""

    for profile in "${profiles[@]}"; do
        echo "==> $lockfile: resolving with $profile"

        conan lock create . \
            "${options[@]}" \
            -pr:b="./profiles/build/${profile}" \
            -pr:h="./profiles/host/${profile}" \
            -s build_type=Release \
            --lockfile="$base" \
            --lockfile-out="$lockfile"

        base="$lockfile"
    done
done

echo "Lockfiles updated. Review 'git diff' before committing."
