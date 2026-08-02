#!/usr/bin/env bash
#
# Records which backend later builds should use for one subsystem.
#
# Kept in a file rather than a VS Code input so the choice survives: an
# input would ask again on every single build.
# The files are per-developer and untracked, like the build folder they
# end up configuring.
#
# Input is deliberately not selectable here.  Its option defaults to
# "auto" and follows graphics, which is the arrangement conanfile.py
# explains; anyone wanting the two apart is past what one picked value
# can express and wants the conan install by hand.
#
# Whether the named directory implements the named subsystem is not
# checked here, because backends/CMakeLists.txt already refuses that at
# configure time and names the actual mistake.  A second copy of that
# rule could only ever disagree with it.

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

subsystem=${1:-}
backend=${2:-}

if [ -z "$subsystem" ] || [ -z "$backend" ]; then
    echo "Usage: scripts/select_backend.sh <gfx|sound|network> <backend>" >&2
    exit 1
fi

case "$subsystem" in
    gfx | sound | network) ;;
    *)
        echo "Unknown subsystem '$subsystem'." >&2
        echo "Available: gfx, sound, network" >&2
        exit 1
        ;;
esac

if [ ! -d "backends/$backend" ]; then
    available=$(find backends -mindepth 1 -maxdepth 1 -type d \
        -printf '%f\n' | sort | tr '\n' ' ')

    echo "Unknown $subsystem backend '$backend'." >&2
    echo "Available: $available" >&2
    exit 1
fi

mkdir -p .vscode
printf '%s\n' "$backend" > ".vscode/$subsystem-backend"

echo "$subsystem backend set to '$backend'."
echo "Ctrl+Shift+B now builds it."
