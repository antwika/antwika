#!/bin/bash

source /usr/local/lib/devcontainer/info.sh

echo "Dev Container"

echo "System:"

info "OS:" "$(. /etc/os-release && echo "$PRETTY_NAME")"
info "User:" "$(whoami)"

echo "Build tools:"

if command -v make >/dev/null 2>&1; then
    info "Make:" "$(make --version | head -n1)"
fi

if command -v cmake >/dev/null 2>&1; then
    info "CMake:" "$(cmake --version | head -n1)"
fi

if command -v ninja >/dev/null 2>&1; then
    info "Ninja:" "$(ninja --version)"
fi

if command -v pkg-config >/dev/null 2>&1; then
    info "pkg-config:" "$(pkg-config --version)"
fi

info "Toolchain:" "${CPP_TOOLCHAIN:-unknown}"

echo "Development tools:"

if command -v git >/dev/null 2>&1; then
    info "Git:" "$(git --version)"
fi

if command -v python3 >/dev/null 2>&1; then
    info "Python:" "$(python3 --version)"
fi

if command -v pipx >/dev/null 2>&1; then
    info "pipx:" "$(pipx --version)"
fi

if command -v conan >/dev/null 2>&1; then
    info "conan:" "$(conan --version)"
fi

echo "Debuggers:"

if command -v gdb >/dev/null 2>&1; then
    info "GDB:" "$(gdb --version | head -n1)"
fi

if command -v lldb >/dev/null 2>&1; then
    info "LLDB:" "$(lldb --version | head -n1)"
fi
