#!/usr/bin/env python3
# Plain-assert tests for check_readme_modules.py.
# Run directly:
#   python3 scripts/tests/test_check_readme_modules.py
import importlib.util
import io
import sys
import tempfile
from contextlib import redirect_stdout
from pathlib import Path

SCRIPT_PATH = (
    Path(__file__).resolve().parent.parent / "check_readme_modules.py"
)

spec = importlib.util.spec_from_file_location(
    "check_readme_modules", SCRIPT_PATH
)
check_readme_modules = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check_readme_modules)
m = check_readme_modules

TREE = """\
```
src/
├── apps/
│   ├── game/
│   └── life/
└── libs/
    ├── engine/
    └── time/
backends/
├── null/
└── sockets/
blog/
```

- `build/bin/antwika_game/antwika_game` -- a game.
- `build/bin/antwika_life/antwika_life` -- a board.
"""


def write(path: Path, content: str = "") -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)


def make_root(tmp: str, readme: str = TREE) -> Path:
    root = Path(tmp)
    write(root / "README.md", readme)
    write(
        root / "src/libs/CMakeLists.txt",
        "add_subdirectory(time)\nadd_subdirectory(engine)\n",
    )
    write(
        root / "src/apps/CMakeLists.txt",
        "add_subdirectory(game)\nadd_subdirectory(life)\n",
    )
    write(root / "backends/null/CMakeLists.txt")
    write(root / "backends/sockets/CMakeLists.txt")
    return root


def run_main(root: Path):
    old_argv = sys.argv
    sys.argv = ["check_readme_modules.py", "--root", str(root)]
    stdout = io.StringIO()
    try:
        with redirect_stdout(stdout):
            exit_code = m.main()
    finally:
        sys.argv = old_argv

    return exit_code, stdout.getvalue()


def it_reads_add_subdirectory_names_in_sorted_order():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "CMakeLists.txt"
        write(
            path,
            "add_subdirectory(time)\n"
            "# add_subdirectory(commented_out)\n"
            "add_subdirectory(engine)\n",
        )

        assert m.subdirectories_of(path) == ["engine", "time"]


def it_reads_the_tree_out_of_the_fenced_block():
    with tempfile.TemporaryDirectory() as tmp:
        root = make_root(tmp)

        children = m.tree_children(root / "README.md")

        assert children["apps"] == ["game", "life"]
        assert children["libs"] == ["engine", "time"]
        assert children["backends"] == ["null", "sockets"]


def it_ignores_a_module_named_only_in_prose():
    # Prose below the tree is how the backends list went stale before.
    readme = TREE.replace(
        "blog/\n```",
        "blog/\n```\n\n`backends/raylib` is discussed here and nowhere else.",
    )

    with tempfile.TemporaryDirectory() as tmp:
        root = make_root(tmp, readme)
        write(root / "backends/raylib/CMakeLists.txt")

        problems = m.find_problems(root)

        assert len(problems) == 1
        assert "'raylib' is missing" in problems[0]


def it_reports_a_library_the_readme_never_lists():
    with tempfile.TemporaryDirectory() as tmp:
        root = make_root(tmp)
        write(
            root / "src/libs/CMakeLists.txt",
            "add_subdirectory(time)\n"
            "add_subdirectory(engine)\n"
            "add_subdirectory(pattern)\n",
        )

        problems = m.find_problems(root)

        assert problems == ["src/libs: 'pattern' is missing from README.md"]


def it_reports_a_readme_entry_no_module_builds():
    with tempfile.TemporaryDirectory() as tmp:
        root = make_root(tmp)
        write(
            root / "src/libs/CMakeLists.txt",
            "add_subdirectory(engine)\n",
        )

        problems = m.find_problems(root)

        assert problems == [
            "src/libs: README.md lists 'time', which is gone"
        ]


def it_reports_an_application_with_no_binary_bullet():
    readme = TREE.replace(
        "- `build/bin/antwika_life/antwika_life` -- a board.\n", ""
    )

    with tempfile.TemporaryDirectory() as tmp:
        root = make_root(tmp, readme)

        problems = m.find_problems(root)

        assert problems == [
            "binaries: README.md never names "
            "'build/bin/antwika_life/antwika_life'"
        ]


def it_succeeds_on_a_readme_that_matches_the_tree():
    with tempfile.TemporaryDirectory() as tmp:
        root = make_root(tmp)

        exit_code, stdout = run_main(root)

        assert exit_code == 0
        assert "OK: README.md names every library" in stdout


def it_fails_and_lists_every_drift():
    with tempfile.TemporaryDirectory() as tmp:
        root = make_root(tmp)
        write(
            root / "src/apps/CMakeLists.txt",
            "add_subdirectory(game)\n"
            "add_subdirectory(life)\n"
            "add_subdirectory(music_editor)\n",
        )

        exit_code, stdout = run_main(root)

        assert exit_code == 1
        assert "'music_editor' is missing from README.md" in stdout
        assert "antwika_music_editor/antwika_music_editor" in stdout


def it_agrees_with_this_repository():
    exit_code, _stdout = run_main(m.DEFAULT_ROOT)

    assert exit_code == 0


def main():
    tests = [
        it_reads_add_subdirectory_names_in_sorted_order,
        it_reads_the_tree_out_of_the_fenced_block,
        it_ignores_a_module_named_only_in_prose,
        it_reports_a_library_the_readme_never_lists,
        it_reports_a_readme_entry_no_module_builds,
        it_reports_an_application_with_no_binary_bullet,
        it_succeeds_on_a_readme_that_matches_the_tree,
        it_fails_and_lists_every_drift,
        it_agrees_with_this_repository,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
