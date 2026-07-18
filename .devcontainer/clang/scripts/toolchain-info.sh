#!/bin/bash

source /usr/local/lib/devcontainer/info.sh

echo "Toolchain info:"

if command -v clang >/dev/null 2>&1; then
    info "clang:" "$(clang --version | head -n1)"
fi

if command -v clang++ >/dev/null 2>&1; then
    info "clang++:" "$(clang++ --version | head -n1)"
fi
