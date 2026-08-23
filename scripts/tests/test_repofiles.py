#!/usr/bin/env python3

import importlib.util
import sys
import tempfile
from pathlib import Path

SCRIPT_PATH = Path(__file__).resolve().parent.parent / "repofiles.py"

sys.path.insert(0, str(SCRIPT_PATH.parent))

spec = importlib.util.spec_from_file_location("repofiles", SCRIPT_PATH)
repofiles = importlib.util.module_from_spec(spec)
spec.loader.exec_module(repofiles)


def write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)


def test_find_paths_reads_a_matching_file() -> None:
    with tempfile.TemporaryDirectory() as name:
        root = Path(name)
        write(root / "src" / "libs" / "one" / "One.hpp", "")

        found = repofiles.find_paths(root, "src/**/*.hpp")

        assert found == [root / "src" / "libs" / "one" / "One.hpp"], found


def test_find_paths_leaves_out_a_build_directory() -> None:
    with tempfile.TemporaryDirectory() as name:
        root = Path(name)
        write(root / "src" / "Kept.hpp", "")
        write(root / "build" / "src" / "Dropped.hpp", "")
        write(root / "build-coverage" / "src" / "Dropped.hpp", "")

        found = repofiles.find_paths(root, "**/*.hpp")

        assert found == [root / "src" / "Kept.hpp"], found


def test_find_paths_keeps_a_file_whose_own_name_starts_with_build() -> None:
    with tempfile.TemporaryDirectory() as name:
        root = Path(name)
        write(root / "src" / "buildOne.hpp", "")

        found = repofiles.find_paths(root, "src/*.hpp")

        assert found == [root / "src" / "buildOne.hpp"], found


def test_find_paths_sorts_what_it_returns() -> None:
    with tempfile.TemporaryDirectory() as name:
        root = Path(name)

        for stem in ("Three", "One", "Two"):
            write(root / "src" / f"{stem}.hpp", "")

        found = repofiles.find_paths(root, "src/*.hpp")

        assert found == sorted(found), found


def test_the_globs_name_the_trees_the_checkers_read() -> None:
    assert "src/**/*.cpp" in repofiles.CPP_GLOBS
    assert "src/**/*.hpp" in repofiles.CPP_GLOBS
    assert "backends/**/*.cpp" in repofiles.CPP_GLOBS
    assert "backends/**/*.hpp" in repofiles.CPP_GLOBS
    assert "scripts/*.py" in repofiles.PYTHON_GLOBS
    assert "CMakeLists.txt" in repofiles.CMAKE_GLOBS
    assert repofiles.MARKDOWN_GLOBS == ("README.md",)


def test_the_default_root_holds_the_checkers() -> None:
    assert (repofiles.DEFAULT_ROOT / "scripts" / "repofiles.py").is_file()


def main() -> int:
    for name, run in sorted(globals().items()):
        if name.startswith("test_") and callable(run):
            run()

    print("OK: repofiles finds what the checkers ask it for.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
