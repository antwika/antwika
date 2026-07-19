#!/bin/bash

source /usr/local/lib/devcontainer/info.sh

echo "Toolchain info:"

if command -v gcc >/dev/null 2>&1; then
    info "gcc:" "$(gcc --version | head -n1)"
fi

if command -v g++ >/dev/null 2>&1; then
    info "g++:" "$(g++ --version | head -n1)"
fi
