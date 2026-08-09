#!/usr/bin/env bash

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
