#!/usr/bin/env bash
#
# Builds, configures and tests one graphics backend.
#
# The backend is whichever one .vscode/gfx-backend names, so the choice
# is made once and every later build honours it.
# Missing file means the null backend, which is what a checkout builds
# before anybody has chosen anything.
#
# The non-null path deliberately mirrors the gfx-backends CI job: same
# build folder, same per-backend lockfile, same preset name. A local
# build that disagrees with CI is worse than no local build at all.

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

selection_file=.vscode/gfx-backend
backend=null

if [ -f "$selection_file" ]; then
    backend=$(tr -d '[:space:]' < "$selection_file")
fi

if [ ! -d "backends/$backend" ]; then
    available=$(find backends -mindepth 1 -maxdepth 1 -type d \
        -printf '%f ' | sort)

    echo "Unknown gfx backend '$backend' in $selection_file." >&2
    echo "Available: $available" >&2
    exit 1
fi

if [ -z "${CONAN_PROFILE:-}" ]; then
    echo "CONAN_PROFILE is not set." >&2
    echo "Set it to a profile name under profiles/host/." >&2
    exit 1
fi

profile_args=(
    "-pr:b=./profiles/build/${CONAN_PROFILE}"
    "-pr:h=./profiles/host/${CONAN_PROFILE}"
)

echo "==> Building the '$backend' gfx backend"

# The tracked CMakePresets.json includes build/CMakePresets.json
# unconditionally, so the default configuration has to be installed
# before CMake will read any preset at all -- even one belonging to a
# different build folder.
conan install . \
    -of build \
    "${profile_args[@]}" \
    --build=missing \
    -s build_type=Release \
    --lockfile=conan.lock

build_dir=build
preset=conan-release

if [ "$backend" != "null" ]; then
    build_dir="build-$backend"
    preset="conan-gfx_backend_${backend}-release"
    layout_conf="tools.cmake.cmake_layout:build_folder_vars"

    conan install . \
        -of "$build_dir" \
        -o "gfx_backend=$backend" \
        -c "${layout_conf}=['options.gfx_backend']" \
        "${profile_args[@]}" \
        --build=missing \
        -s build_type=Release \
        --lockfile="conan-${backend}.lock"
fi

cmake --preset "$preset"
cmake --build "$build_dir" -j24
ctest --test-dir "$build_dir" --output-on-failure
