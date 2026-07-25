#!/bin/bash

source /usr/local/lib/devcontainer/info.sh

echo "Toolchain info:"

if command -v clang >/dev/null 2>&1; then
    info "clang:" "$(clang --version | head -n1)"
fi

if command -v clang++ >/dev/null 2>&1; then
    info "clang++:" "$(clang++ --version | head -n1)"
fi

if command -v llvm-cov-21 >/dev/null 2>&1; then
    info "llvm-cov-21:" "$(llvm-cov-21 --version | head -n1)"
fi

if dpkg-query -W -f='${Status}' libclang-rt-21-dev 2>/dev/null | grep -q "install ok installed"; then
    info "libclang-rt-21-dev:" "$(dpkg-query -W -f='${Version}' libclang-rt-21-dev)"
fi
