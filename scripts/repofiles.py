#!/usr/bin/env python3

from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent

CPP_GLOBS = (
    "src/**/*.cpp",
    "src/**/*.hpp",
    "backends/**/*.cpp",
    "backends/**/*.hpp",
    "benchmarks/**/*.cpp",
    "benchmarks/**/*.hpp",
)

PYTHON_GLOBS = (
    "scripts/*.py",
    "scripts/tests/*.py",
    "conanfile.py",
    "src/libs/*/conanfile.py",
)

CMAKE_GLOBS = (
    "cmake/*.cmake",
    "CMakeLists.txt",
    "src/**/CMakeLists.txt",
    "backends/**/CMakeLists.txt",
)

MARKDOWN_GLOBS = ("README.md",)

YAML_GLOBS = (".github/workflows/*.yml",)

SHELL_GLOBS = (
    "scripts/*.sh",
    ".devcontainer/*/scripts/*.sh",
)

DOCKER_GLOBS = (".devcontainer/*/Dockerfile",)


def find_paths(root: Path, pattern: str) -> list:
    return sorted(
        path
        for path in root.glob(pattern)
        if not any(
            part.startswith("build")
            for part in path.relative_to(root).parts[:-1]
        )
    )
