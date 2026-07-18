#!/bin/bash

source /usr/local/lib/devcontainer/info.sh

echo "Toolchain info:"

if command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
    info "x86_64-w64-mingw32-g++" "$(x86_64-w64-mingw32-g++ --version | head -n1)"
fi
