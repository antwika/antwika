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

# The frameworks are the recipe's own gfx_backend values rather than
# the directories under backends/, because those two stopped being the
# same list: backends/sockets names the operating system's own socket
# API, adds no package and no lockfile entry, and is not a legal
# gfx_backend at all -- so a loop over the directory listing asked
# Conan for a configuration it refuses, after the real lockfiles had
# already been rewritten.
# gfx_backend is the widest of the four options, since input names the
# same values and sound names a subset, which is why one option answers
# for all of them.
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

# One lockfile per framework, not per subsystem.
# Selecting a directory for graphics, for input or for sound adds the
# same requirement to the graph, so -o sound_backend=sdl3 resolves
# against conan-sdl3.lock exactly as -o gfx_backend=sdl3 does.
# Naming the framework once here is what keeps that true.
# "null" leads the list because the default configuration's lockfile is
# the one every framework-free build reads, sockets included.
for backend in null "${frameworks[@]}"; do
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
