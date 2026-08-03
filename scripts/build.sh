#!/usr/bin/env bash
#
# Configures, builds and tests the selected backends.
#
# The selections are whichever backends .vscode/gfx-backend,
# .vscode/sound-backend and .vscode/network-backend name, so each
# choice is made once and every later build honours it.
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

    # An empty selection would test backends/ itself, which exists,
    # and the run would die much later asking for 'conan-.lock'.
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

# A lockfile is per framework rather than per subsystem, and one build
# may name at most one framework -- a rule conanfile.py's validate() and
# backends/CMakeLists.txt both enforce.
# Refusing it here too is what keeps that refusal from arriving as a
# lockfile that does not hold what the graph went on to ask for.
#
# 'sockets' is exempt for the reason both of those give: it names the
# operating system's own socket API rather than a framework, so it adds
# no package, no lockfile entry and no event queue to compete for.
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

# The host profile is the tracked record of this machine's compiler,
# and Conan trusts it without looking.
# A container image bump that outruns the profile would label every
# cached dependency with a version nothing actually ran, and the CI
# cache key -- which hashes only conanfile, locks and profiles --
# would never notice; failing here is what makes the next bump loud.
# The binary asked is the profile's own: the conf's named executable
# when the profile carries one (MinGW does), and otherwise whatever
# the compiler= line says, which is a command in every container.
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

# Everything from the first non-digit on is dropped rather than
# everything from the first dot, because -dumpversion does not always
# answer with a dotted version at all: Debian's MinGW cross compiler
# says "13-win32", naming the thread model it was built with, and
# cutting that on '.' leaves it whole and unequal to every profile.
# Conan's compiler.version is the major number alone, which is exactly
# the leading digits of every form of that answer.
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

# A dev container has neither a display, a sound card nor anything to
# talk to, so a real backend's conformance suite needs a headless
# runner.
# The suite is split in two rather than run under one, which is the
# same split the backends CI job makes and for the same reason: sound
# and network run with no display at all, deliberately, since a backend
# of either kind that needed Xvfb would be one that had quietly taken a
# dependency on video, and running it under Xvfb is exactly what would
# hide that.
# Splitting unconditionally is what keeps that from being a rule only
# some selections obey -- the null suites cost under a second.
headless_suites='SoundBackendConformance|NetworkBackendConformance'

# --no-tests=error on both halves, because a -R or -E that matches
# nothing is otherwise a pass.
# A suite renamed out from under this script would leave the half that
# names it running zero tests and exiting 0 for good, and a build
# script silently testing nothing is the one failure it cannot report
# on its own behalf.
# Neither half is ever legitimately empty, whatever is selected: the
# null sound and network backends ship conformance suites of their own
# under src/libs/, so a match of zero means the name here is wrong
# rather than that nothing was selected.
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
