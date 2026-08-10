#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

read_selection() {
    local subsystem=$1
    local file=.vscode/$subsystem-backend
    local backend=null

    if [ -f "$file" ]; then
        backend=$(tr -d '[:space:]' < "$file")
    fi

    if [ -z "$backend" ] || [ ! -d "backends/$backend" ]; then
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
network_backend=$(read_selection network)

framework=null

for backend in "$gfx_backend" "$sound_backend" "$network_backend"; do
    if [ "$backend" != "null" ] && [ "$backend" != "sockets" ]; then
        if [ "$framework" != "null" ] && [ "$framework" != "$backend" ]; then
            echo "gfx backend '$gfx_backend', sound backend" >&2
            echo "'$sound_backend' and network backend" >&2
            echo "'$network_backend' name two different frameworks." >&2
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

host_profile=profiles/host/${CONAN_PROFILE}

profile_compiler() {
    local named
    named=$(sed -n 's/.*"c" *: *"\([^"]*\)".*/\1/p' "$host_profile")

    if [ -n "$named" ]; then
        printf '%s' "$named"
    else
        sed -n 's/^compiler=//p' "$host_profile"
    fi
}

expected_version=$(sed -n 's/^compiler.version=//p' "$host_profile")
compiler=$(profile_compiler)
actual_version=$("$compiler" -dumpversion | sed 's/[^0-9].*//')

if [ "$expected_version" != "$actual_version" ]; then
    echo "$host_profile says compiler.version=$expected_version," >&2
    echo "and $compiler reports $actual_version." >&2
    echo "Update the profile (and re-run" >&2
    echo "scripts/update_lockfiles.sh) so the record matches the" >&2
    echo "toolchain." >&2
    exit 1
fi

echo "==> gfx backend '$gfx_backend', sound backend '$sound_backend',"
echo "==> network backend '$network_backend'"

conan install . \
    -of build \
    -o "gfx_backend=$gfx_backend" \
    -o "sound_backend=$sound_backend" \
    -o "network_backend=$network_backend" \
    "-pr:b=./profiles/build/${CONAN_PROFILE}" \
    "-pr:h=./profiles/host/${CONAN_PROFILE}" \
    --build=missing \
    -s build_type=Release \
    --lockfile="$lockfile"

cmake --preset conan-release
cmake --build build -j"$(nproc)"

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

headless_suites='SoundBackendConformance|NetworkBackendConformance'

echo "==> Sound and network suites, with no display"
SDL_AUDIO_DRIVER=dummy \
    ctest --test-dir build --output-on-failure --no-tests=error \
    -R "$headless_suites"

echo "==> Everything else"

if [ "$gfx_backend" = "null" ]; then
    ctest --test-dir build --output-on-failure --no-tests=error \
        -E "$headless_suites"
else
    xvfb-run -a ctest --test-dir build --output-on-failure \
        --no-tests=error -E "$headless_suites"
fi
