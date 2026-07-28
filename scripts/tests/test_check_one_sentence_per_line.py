#!/usr/bin/env python3
# Plain-assert tests for check_one_sentence_per_line.py.
# Run directly:
#   python3 scripts/tests/test_check_one_sentence_per_line.py
import importlib.util
import sys
import tempfile
from pathlib import Path

SCRIPT_PATH = Path(__file__).resolve().parent.parent / "check_one_sentence_per_line.py"

spec = importlib.util.spec_from_file_location("check_one_sentence_per_line", SCRIPT_PATH)
check_one_sentence_per_line = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check_one_sentence_per_line)


def write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)


def it_detects_two_sentences_sharing_a_line():
    assert check_one_sentence_per_line.has_multiple_sentences("First sentence. Second sentence.") is True


def it_allows_a_single_long_sentence():
    text = "A single sentence that just happens to run on for quite a while, with several clauses, is still fine."
    assert check_one_sentence_per_line.has_multiple_sentences(text) is False


def it_does_not_treat_abbreviations_as_sentence_ends():
    assert check_one_sentence_per_line.has_multiple_sentences("Uses a mock, e.g. MockFoo, for isolation.") is False


def it_does_not_treat_a_dotted_identifier_inside_a_code_span_as_a_sentence_end():
    assert check_one_sentence_per_line.has_multiple_sentences("See `Foo.Bar` for details.") is False


def it_ignores_a_leading_numbered_list_marker():
    assert check_one_sentence_per_line.has_multiple_sentences("1. Do the first thing.") is False


def it_treats_a_period_followed_by_space_and_capital_as_a_break():
    assert check_one_sentence_per_line.ends_sentence("This line ends properly.") is True


def it_treats_a_missing_terminator_as_not_ending():
    assert check_one_sentence_per_line.ends_sentence("this line just trails off") is False


def it_ignores_trailing_decoration_when_checking_the_terminator():
    assert check_one_sentence_per_line.ends_sentence('**Bold sentence.**') is True


def it_recognizes_a_badge_only_line_as_pure_markup():
    text = "[![CI](https://img.shields.io/badge/CI-passing-green)](https://example.com/ci)"
    assert check_one_sentence_per_line.is_pure_markup(text) is True


def it_does_not_treat_a_sentence_containing_a_link_as_pure_markup():
    text = "See [the docs](https://example.com) for details."
    assert check_one_sentence_per_line.is_pure_markup(text) is False


def it_flags_a_single_comment_line_with_two_sentences():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "Foo.cpp"
        write(path, "// First sentence. Second sentence.\nint x;\n")

        violations = check_one_sentence_per_line.check_cpp_file(path)

        assert len(violations) == 1
        assert violations[0].kind == "multiple"
        assert violations[0].line == 1


def it_flags_a_sentence_wrapped_across_two_comment_lines():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "Foo.cpp"
        write(path, "// This sentence keeps going\n// onto a second line.\nint x;\n")

        violations = check_one_sentence_per_line.check_cpp_file(path)

        assert len(violations) == 1
        assert violations[0].kind == "wrapped"
        assert violations[0].line == 1


def it_allows_a_properly_split_two_sentence_comment_block():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "Foo.cpp"
        write(path, "// First sentence.\n// Second sentence.\nint x;\n")

        assert check_one_sentence_per_line.check_cpp_file(path) == []


def it_does_not_flag_an_inline_trailing_comment_fragment():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "Foo.cpp"
        write(path, "int x; // FNV-1a prime\nint y; // FNV-1a offset basis\n")

        assert check_one_sentence_per_line.check_cpp_file(path) == []


def it_does_not_follow_a_url_scheme_into_a_comment():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "Foo.cpp"
        write(path, 'const char *url = "https://example.com";\n')

        assert check_one_sentence_per_line.check_cpp_file(path) == []


def it_flags_a_sentence_wrapped_across_two_markdown_lines():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "doc.md"
        write(path, "This sentence keeps going\nonto a second line.\n")

        violations = check_one_sentence_per_line.check_markdown_file(path)

        assert len(violations) == 1
        assert violations[0].kind == "wrapped"


def it_does_not_flag_consecutive_short_list_items():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "doc.md"
        write(path, "- **GNU Dev Container**\n- **LLVM Dev Container**\n- **MinGW Dev Container**\n")

        assert check_one_sentence_per_line.check_markdown_file(path) == []


def it_does_not_flag_a_multi_line_badge_block():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "doc.md"
        write(
            path,
            "[![CI](https://img.shields.io/badge/CI-passing-green)](https://example.com/ci)\n"
            "[![Coverage](https://img.shields.io/badge/coverage-99%25-green)](https://example.com/cov)\n",
        )

        assert check_one_sentence_per_line.check_markdown_file(path) == []


def it_does_not_scan_inside_fenced_code_blocks():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "doc.md"
        write(path, "```sh\nsome command\nspread across lines\n```\n")

        assert check_one_sentence_per_line.check_markdown_file(path) == []


def it_allows_a_multi_sentence_list_item_split_across_its_own_lines():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "doc.md"
        write(path, "1. First sentence of the item.\n   Second sentence of the same item.\n2. A different item.\n")

        assert check_one_sentence_per_line.check_markdown_file(path) == []


MAX_COMMENT_LENGTH = 80


def _comment_texts(path: Path, marker: str):
    texts = []
    for line_no, raw in enumerate(path.read_text(errors="ignore").splitlines(), start=1):
        stripped = raw.strip()
        if stripped.startswith(marker) and (marker != "#" or not stripped.startswith("#!")):
            comment = stripped[len(marker):].strip()
            if comment:
                texts.append((line_no, comment))
            continue
        idx = check_one_sentence_per_line._find_comment_start(raw, marker)
        if idx is not None:
            comment = raw[idx + len(marker):].strip()
            if comment:
                texts.append((line_no, comment))
    return texts


def it_keeps_every_comment_at_or_under_eighty_characters():
    root = check_one_sentence_per_line.DEFAULT_ROOT
    too_long = []

    for pattern in check_one_sentence_per_line.CPP_GLOBS:
        for path in sorted(root.glob(pattern)):
            for line_no, comment in _comment_texts(path, "//"):
                if len(comment) > MAX_COMMENT_LENGTH:
                    too_long.append(f"{path}:{line_no}: {len(comment)} chars: {comment}")

    for pattern in check_one_sentence_per_line.PYTHON_GLOBS:
        for path in sorted(root.glob(pattern)):
            for line_no, comment in _comment_texts(path, "#"):
                if len(comment) > MAX_COMMENT_LENGTH:
                    too_long.append(f"{path}:{line_no}: {len(comment)} chars: {comment}")

    assert too_long == [], "\n".join(too_long)


def it_finds_violations_across_the_configured_file_globs():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "README.md", "This wraps\nonto a second line.\n")
        write(root / "src/libs/foo/src/Foo.cpp", "// Fine as-is.\nint x;\n")

        violations = check_one_sentence_per_line.find_violations(root)

        assert len(violations) == 1
        assert violations[0].path == root / "README.md"


def main():
    tests = [
        it_detects_two_sentences_sharing_a_line,
        it_allows_a_single_long_sentence,
        it_does_not_treat_abbreviations_as_sentence_ends,
        it_does_not_treat_a_dotted_identifier_inside_a_code_span_as_a_sentence_end,
        it_ignores_a_leading_numbered_list_marker,
        it_treats_a_period_followed_by_space_and_capital_as_a_break,
        it_treats_a_missing_terminator_as_not_ending,
        it_ignores_trailing_decoration_when_checking_the_terminator,
        it_recognizes_a_badge_only_line_as_pure_markup,
        it_does_not_treat_a_sentence_containing_a_link_as_pure_markup,
        it_flags_a_single_comment_line_with_two_sentences,
        it_flags_a_sentence_wrapped_across_two_comment_lines,
        it_allows_a_properly_split_two_sentence_comment_block,
        it_does_not_flag_an_inline_trailing_comment_fragment,
        it_does_not_follow_a_url_scheme_into_a_comment,
        it_flags_a_sentence_wrapped_across_two_markdown_lines,
        it_does_not_flag_consecutive_short_list_items,
        it_does_not_flag_a_multi_line_badge_block,
        it_does_not_scan_inside_fenced_code_blocks,
        it_allows_a_multi_sentence_list_item_split_across_its_own_lines,
        it_keeps_every_comment_at_or_under_eighty_characters,
        it_finds_violations_across_the_configured_file_globs,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
