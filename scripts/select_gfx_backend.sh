#!/usr/bin/env bash
#
# Records which graphics backend later builds should use.
#
# Kept in a file rather than a VS Code input so the choice survives: an
# input would ask again on every single build.
# The file is per-developer and untracked, like the build folders it
# ends up selecting.

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

backend=${1:-}

if [ -z "$backend" ]; then
    echo "Usage: scripts/select_gfx_backend.sh <backend>" >&2
    exit 1
fi

if [ ! -d "backends/$backend" ]; then
    available=$(find backends -mindepth 1 -maxdepth 1 -type d \
        -printf '%f ' | sort)

    echo "Unknown gfx backend '$backend'." >&2
    echo "Available: $available" >&2
    exit 1
fi

mkdir -p .vscode
printf '%s\n' "$backend" > .vscode/gfx-backend

echo "gfx backend set to '$backend'."
echo "Ctrl+Shift+B now builds it."
