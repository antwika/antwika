#!/usr/bin/env bash
#
# Re-resolves every tracked Conan lockfile against conanfile.py.
#
# Renovate bumps a version in conanfile.py but leaves the lockfiles
# pinned to the old one, so the graph has to be re-resolved by hand
# before such a change builds.
# Each backend gets its own lockfile, because selecting a backend
# changes the dependency graph.
# Every profile is resolved into the same lockfile, since CI builds all
# of them against these files; the first pass starts from an empty
# lockfile so stale pins cannot survive, and the later ones merge into
# what it wrote.

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

mapfile -t profiles < <(
    find profiles/host -mindepth 1 -maxdepth 1 -type f -printf '%f\n' \
        | sort
)

if [ "${#profiles[@]}" -eq 0 ]; then
    echo "No profiles found under profiles/host." >&2
    exit 1
fi

mapfile -t backends < <(
    find backends -mindepth 1 -maxdepth 1 -type d -printf '%f\n' | sort
)

for backend in "${backends[@]}"; do
    options=()
    lockfile=conan.lock

    if [ "$backend" != "null" ]; then
        options=(-o "gfx_backend=$backend")
        lockfile="conan-${backend}.lock"
    fi

    # An empty --lockfile is Conan's way of saying "resolve from
    # scratch", which is the whole point of the first pass.
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
