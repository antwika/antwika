#!/usr/bin/env python3
# Plain-assert tests for check_unused_test_doubles.py. Run directly:
#   python3 scripts/tests/test_check_unused_test_doubles.py
import importlib.util
import io
import sys
import tempfile
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

SCRIPT_PATH = Path(__file__).resolve().parent.parent / "check_unused_test_doubles.py"

spec = importlib.util.spec_from_file_location("check_unused_test_doubles", SCRIPT_PATH)
check_unused_test_doubles = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check_unused_test_doubles)


def write(path: Path, content: str = "") -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)


def run_main(root: Path):
    old_argv = sys.argv
    sys.argv = ["check_unused_test_doubles.py", "--root", str(root)]
    stdout, stderr = io.StringIO(), io.StringIO()
    try:
        with redirect_stdout(stdout), redirect_stderr(stderr):
            exit_code = check_unused_test_doubles.main()
    finally:
        sys.argv = old_argv

    return exit_code, stdout.getvalue(), stderr.getvalue()


def it_finds_mock_headers():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        mock = root / "src/libs/foo/tests/mocks/include/antwika/foo/mocks/MockFoo.hpp"
        unrelated = root / "src/libs/foo/include/antwika/foo/Foo.hpp"
        write(mock)
        write(unrelated)

        doubles = check_unused_test_doubles.find_test_doubles(root)

        assert doubles == [mock]


def it_finds_fake_headers():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        fake = root / "src/libs/bar/tests/fakes/include/antwika/bar/fakes/FakeBar.hpp"
        unrelated = root / "src/libs/bar/include/antwika/bar/Bar.hpp"
        write(fake)
        write(unrelated)

        doubles = check_unused_test_doubles.find_test_doubles(root)

        assert doubles == [fake]


def it_detects_a_referenced_mock_header():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "src/libs/foo/tests/FooTest.cpp", '#include "antwika/foo/mocks/MockFoo.hpp"\n')

        assert check_unused_test_doubles.is_included_anywhere("MockFoo.hpp", root) is True


def it_detects_a_referenced_fake_header():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "src/libs/foo/tests/FooTest.cpp", '#include "antwika/foo/fakes/FakeFoo.hpp"\n')

        assert check_unused_test_doubles.is_included_anywhere("FakeFoo.hpp", root) is True


def it_fails_when_no_test_doubles_exist():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "src/libs/foo/src/Foo.cpp", "")

        exit_code, _stdout, stderr = run_main(root)

        assert exit_code == 1
        assert "did the layout change?" in stderr


def it_fails_and_lists_unreferenced_mock_headers():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "src/libs/foo/tests/mocks/include/antwika/foo/mocks/MockFoo.hpp")
        write(root / "src/libs/foo/tests/FooTest.cpp", "// no test doubles referenced here\n")

        exit_code, stdout, _stderr = run_main(root)

        assert exit_code == 1
        assert "MockFoo.hpp" in stdout
        assert "never included by any .cpp file" in stdout


def it_succeeds_when_every_test_double_is_referenced():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "src/libs/foo/tests/mocks/include/antwika/foo/mocks/MockFoo.hpp")
        write(root / "src/libs/foo/tests/FooTest.cpp", '#include "antwika/foo/mocks/MockFoo.hpp"\n')

        exit_code, stdout, _stderr = run_main(root)

        assert exit_code == 0
        assert "OK: every mock/fake header is included by at least one .cpp file (1 checked)." in stdout


def main():
    tests = [
        it_finds_mock_headers,
        it_finds_fake_headers,
        it_detects_a_referenced_mock_header,
        it_detects_a_referenced_fake_header,
        it_fails_when_no_test_doubles_exist,
        it_fails_and_lists_unreferenced_mock_headers,
        it_succeeds_when_every_test_double_is_referenced,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
