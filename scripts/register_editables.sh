#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

source scripts/packages.sh

for editable in "${ANTWIKA_EDITABLES[@]}"; do
    conan editable add "$editable"
done
