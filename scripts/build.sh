#!/usr/bin/env bash
#
# Configures, builds and tests the selected backends.
#
# The selections are whichever backends .vscode/gfx-backend and
# .vscode/sound-backend name, so each choice is made once and every
# later build honours it.
# A missing file means null, which is what a checkout builds before
# anybody has chosen anything.
#
# Everything lands in build/ under the conan-release preset, whatever
# is selected, which is why no build_folder_vars conf appears below:
# that conf exists to put the backend in the folder and preset names,
# and one folder is exactly the case that does not need it.
# CI gives each backend a folder of its own because its legs run in
# parallel and cache separately.  A developer switches between them one
# at a time, so per-permutation folders were mostly stale trees.
# The cost is that switching reconfigures and largely rebuilds, since
# the selections are cache variables deciding what backends/ compiles.
#
# This is also why the separate "install the default configuration for
# its presets" step CI needs is absent: the folder installed here is
# build/, so the presets the tracked CMakePresets.json includes are
# always the ones just written.

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

read_selection() {
    local subsystem=$1
    local file=.vscode/$subsystem-backend
    local backend=null

    if [ -f "$file" ]; then
        backend=$(tr -d '[:space:]' < "$file")
    fi

    if [ ! -d "backends/$backend" ]; then
        local available
        available=$(find backends -mindepth 1 -maxdepth 1 -type d \
            -printf '%f\n' | sort | tr '\n' ' ')

        echo "Unknown $subsystem backend '$backend' in $file." >&2
        echo "Available: $available" >&2
        return 1
    fi

    printf '%s' "$backend"
}

gfx_backend=$(read_selection gfx)
sound_backend=$(read_selection sound)

# A lockfile is per framework rather than per subsystem, and one build
# may name at most one framework -- a rule conanfile.py's validate() and
# backends/CMakeLists.txt both enforce.
# Refusing it here too is what keeps that refusal from arriving as a
# lockfile that does not hold what the graph went on to ask for.
framework=null

for backend in "$gfx_backend" "$sound_backend"; do
    if [ "$backend" != "null" ]; then
        if [ "$framework" != "null" ] && [ "$framework" != "$backend" ]; then
            echo "gfx backend '$gfx_backend' and sound backend" >&2
            echo "'$sound_backend' name two different frameworks." >&2
            echo "Use one framework per build, or 'null' to opt out." >&2
            exit 1
        fi

        framework=$backend
    fi
done

lockfile=conan.lock

if [ "$framework" != "null" ]; then
    lockfile="conan-$framework.lock"
fi

if [ -z "${CONAN_PROFILE:-}" ]; then
    echo "CONAN_PROFILE is not set." >&2
    echo "Set it to a profile name under profiles/host/." >&2
    exit 1
fi

echo "==> gfx backend '$gfx_backend', sound backend '$sound_backend'"

conan install . \
    -of build \
    -o "gfx_backend=$gfx_backend" \
    -o "sound_backend=$sound_backend" \
    "-pr:b=./profiles/build/${CONAN_PROFILE}" \
    "-pr:h=./profiles/host/${CONAN_PROFILE}" \
    --build=missing \
    -s build_type=Release \
    --lockfile="$lockfile"

cmake --preset conan-release
cmake --build build -j24

# Every tests/ directory is guarded by NOT CMAKE_CROSSCOMPILING, so a
# cross build has no tests to run -- and its executables would not run
# on this machine even if it had.
# Saying so and stopping is what keeps a MinGW build from ending in an
# xvfb-run that was never going to help.
# The two profiles are the same fact CMake crosscompiles on, read from
# the same files the install above already names.
profile_os() {
    sed -n 's/^os=//p' "profiles/$1/${CONAN_PROFILE}"
}

host_os=$(profile_os host)
build_os=$(profile_os build)

if [ "$host_os" != "$build_os" ]; then
    echo "==> Built for $host_os on $build_os, so no tests are run"
    echo "The executables in build/bin/ run on $host_os."
    exit 0
fi

# A dev container has neither a display nor a sound card, so a real
# backend's conformance suite needs a headless runner.
# The suite is split in two rather than run under one, which is the
# same split the gfx-backends CI job makes and for the same reason:
# sound runs with no display at all, deliberately, since a sound
# backend that needed Xvfb would be one that had quietly taken a
# dependency on video, and running it under Xvfb is exactly what would
# hide that.
# Splitting unconditionally is what keeps that from being a rule only
# some selections obey -- the null suites cost under a second, and
# either half is free to be empty.
sound_suite=SoundBackendConformance

echo "==> Sound suites, with no display"
SDL_AUDIO_DRIVER=dummy \
    ctest --test-dir build --output-on-failure -R "$sound_suite"

echo "==> Everything else"

if [ "$gfx_backend" = "null" ]; then
    ctest --test-dir build --output-on-failure -E "$sound_suite"
else
    xvfb-run -a ctest --test-dir build --output-on-failure -E "$sound_suite"
fi
