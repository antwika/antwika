#!/usr/bin/env python3
# Plain-assert tests for check_unused_test_doubles.py.
# Run directly:
#   python3 scripts/tests/test_check_unused_test_doubles.py
import importlib.util
import io
import sys
import tempfile
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

SCRIPT_PATH = (
    Path(__file__).resolve().parent.parent / "check_unused_test_doubles.py"
)

spec = importlib.util.spec_from_file_location(
    "check_unused_test_doubles", SCRIPT_PATH
)
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
        mock = (
            root
            / "src/libs/foo/tests/mocks/include/antwika/foo/mocks/MockFoo.hpp"
        )
        unrelated = root / "src/libs/foo/include/antwika/foo/Foo.hpp"
        write(mock)
        write(unrelated)

        doubles = check_unused_test_doubles.find_test_doubles(root)

        assert doubles == [mock]


def it_finds_fake_headers():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        fake = (
            root
            / "src/libs/bar/tests/fakes/include/antwika/bar/fakes/FakeBar.hpp"
        )
        unrelated = root / "src/libs/bar/include/antwika/bar/Bar.hpp"
        write(fake)
        write(unrelated)

        doubles = check_unused_test_doubles.find_test_doubles(root)

        assert doubles == [fake]


def it_computes_the_include_path_from_a_full_header_path():
    header = Path(
        "/repo/src/libs/foo/tests/mocks/include/antwika/foo/mocks/MockFoo.hpp"
    )

    include_path = check_unused_test_doubles.include_path_for(header)

    assert include_path == "antwika/foo/mocks/MockFoo.hpp"


def it_detects_a_referenced_mock_header():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/foo/tests/FooTest.cpp",
            '#include "antwika/foo/mocks/MockFoo.hpp"\n',
        )

        included = check_unused_test_doubles.collect_included_paths(root)
        is_included = check_unused_test_doubles.is_included_anywhere
        assert is_included("antwika/foo/mocks/MockFoo.hpp", included) is True


def it_detects_a_referenced_fake_header():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/foo/tests/FooTest.cpp",
            '#include "antwika/foo/fakes/FakeFoo.hpp"\n',
        )

        included = check_unused_test_doubles.collect_included_paths(root)
        is_included = check_unused_test_doubles.is_included_anywhere
        assert is_included("antwika/foo/fakes/FakeFoo.hpp", included) is True


def it_detects_an_angle_bracket_include():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/foo/tests/FooTest.cpp",
            "#include <antwika/foo/mocks/MockFoo.hpp>\n",
        )

        included = check_unused_test_doubles.collect_included_paths(root)
        is_included = check_unused_test_doubles.is_included_anywhere
        assert is_included("antwika/foo/mocks/MockFoo.hpp", included) is True


def it_does_not_count_a_bare_comment_mention_as_included():
    # Regression test for is_included_anywhere's old substring bug.
    # A header merely named in a comment used to count as included.
    # That was a false negative, since it was never truly #included.
    # A dead mock/fake could then slip past the check undetected.
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/foo/tests/FooTest.cpp",
            "// MockFoo.hpp was replaced by a fake, remove it\n",
        )

        included = check_unused_test_doubles.collect_included_paths(root)
        is_included = check_unused_test_doubles.is_included_anywhere
        assert is_included("antwika/foo/mocks/MockFoo.hpp", included) is False


def it_does_not_confuse_a_same_named_mock_in_another_module():
    # Regression test for matching by bare filename alone.
    # An unused mock in one module could go undetected this way.
    # A same-named mock genuinely used by a different module masks it.
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/bar/tests/FooTest.cpp",
            '#include "antwika/bar/mocks/MockFoo.hpp"\n',
        )

        included = check_unused_test_doubles.collect_included_paths(root)
        is_included = check_unused_test_doubles.is_included_anywhere
        assert is_included("antwika/foo/mocks/MockFoo.hpp", included) is False
        assert is_included("antwika/bar/mocks/MockFoo.hpp", included) is True


def it_counts_an_include_from_a_backend_suite():
    # backends/ lives outside src/ and its suites use src/'s mocks.
    # A double used only there used to be reported as an orphan.
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "backends/sockets/tests/SocketsHostTest.cpp",
            "#include <antwika/log/mocks/MockLogger.hpp>\n",
        )

        included = check_unused_test_doubles.collect_included_paths(root)
        is_included = check_unused_test_doubles.is_included_anywhere
        assert is_included("antwika/log/mocks/MockLogger.hpp", included)


def it_accepts_a_double_used_only_by_a_backend_suite():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root
            / "src/libs/log/tests/mocks/include/antwika/log/mocks/"
            "MockLogger.hpp"
        )
        write(
            root / "backends/sockets/tests/SocketsHostTest.cpp",
            "#include <antwika/log/mocks/MockLogger.hpp>\n",
        )

        exit_code, stdout, _stderr = run_main(root)

        assert exit_code == 0
        assert "(1 checked)" in stdout


def it_fails_when_no_test_doubles_exist():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "src/libs/foo/src/Foo.cpp", "")

        exit_code, _stdout, stderr = run_main(root)

        assert exit_code == 1
        assert "did the layout change?" in stderr


def it_fails_and_lists_unreferenced_mock_headers():
    # The fixture mentions the mock's filename only in a comment.
    # It's never included as a real #include.
    # The old substring-based check would have wrongly called it used.
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root
            / "src/libs/foo/tests/mocks/include/antwika/foo/mocks/MockFoo.hpp"
        )
        write(
            root / "src/libs/foo/tests/FooTest.cpp",
            "// MockFoo.hpp was replaced by a fake, remove it\n",
        )

        exit_code, stdout, _stderr = run_main(root)

        assert exit_code == 1
        assert "MockFoo.hpp" in stdout
        assert "never included by any .cpp file" in stdout


def it_succeeds_when_every_test_double_is_referenced():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root
            / "src/libs/foo/tests/mocks/include/antwika/foo/mocks/MockFoo.hpp"
        )
        write(
            root / "src/libs/foo/tests/FooTest.cpp",
            '#include "antwika/foo/mocks/MockFoo.hpp"\n',
        )

        exit_code, stdout, _stderr = run_main(root)

        assert exit_code == 0
        assert (
            "OK: every mock/fake header is included by at least one "
            ".cpp file (1 checked)."
        ) in stdout


def main():
    tests = [
        it_finds_mock_headers,
        it_finds_fake_headers,
        it_computes_the_include_path_from_a_full_header_path,
        it_detects_a_referenced_mock_header,
        it_detects_a_referenced_fake_header,
        it_detects_an_angle_bracket_include,
        it_does_not_count_a_bare_comment_mention_as_included,
        it_does_not_confuse_a_same_named_mock_in_another_module,
        it_counts_an_include_from_a_backend_suite,
        it_accepts_a_double_used_only_by_a_backend_suite,
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
