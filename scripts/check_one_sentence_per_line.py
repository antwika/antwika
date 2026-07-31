#!/usr/bin/env python3
# Fails if a comment or markdown paragraph line holds more than one sentence.
# Also fails if a single sentence is wrapped across multiple lines.
# The convention is exactly one sentence per line, however long.
import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent

# Every markdown document in the repository.
# docs/ holds only documents that are still normative, so all of it is
# prose somebody still edits and all of it is checked.
MARKDOWN_GLOBS = (
    "README.md",
    "CLAUDE.md",
    "REQUIREMENTS.md",
    "blog/*.md",
    "docs/**/*.md",
)
# Backends live outside src/ and so outside the coverage gate.
# The style rules still apply to them.
CPP_GLOBS = (
    "src/**/*.cpp",
    "src/**/*.hpp",
    "backends/**/*.cpp",
    "backends/**/*.hpp",
)
PYTHON_GLOBS = ("scripts/*.py", "scripts/tests/*.py")

ABBREVIATIONS = ("e.g.", "i.e.", "etc.", "vs.", "cf.", "approx.")
CODE_SPAN = re.compile(r"`[^`]*`")
SENTENCE_BREAK = re.compile(r'[.!?]\s+[A-Z*`"]')
LIST_MARKER = re.compile(r"^(?:\d+\.|[-*])\s+")
TRAILING_DECORATION = re.compile(r"""[)"'*`]+$""")
FENCE = re.compile(r"^(```|~~~)")


@dataclass
class Violation:
    path: Path
    line: int
    kind: str
    text: str

    def __str__(self) -> str:
        reason = (
            "holds multiple sentences"
            if self.kind == "multiple"
            else "wraps a sentence onto the next line"
        )
        return f"{self.path}:{self.line}: {reason}\n    {self.text}"


def _mask_code_spans(text: str) -> str:
    def mask(m: re.Match[str]) -> str:
        return "`" + "_" * (len(m.group(0)) - 2) + "`"

    return CODE_SPAN.sub(mask, text)


def _mask_abbreviations(text: str) -> str:
    for abbreviation in ABBREVIATIONS:
        text = text.replace(abbreviation, "_" * len(abbreviation))
    return text


def has_multiple_sentences(text: str) -> bool:
    text = _mask_code_spans(text)
    text = LIST_MARKER.sub("", text, count=1)
    text = _mask_abbreviations(text)
    return bool(SENTENCE_BREAK.search(text))


def ends_sentence(text: str) -> bool:
    text = TRAILING_DECORATION.sub("", text.rstrip())
    return text.endswith((".", "!", "?", ":"))


def is_pure_markup(text: str) -> bool:
    # Drops the *contents* of every (...) and [...] group.
    # That removes link/image labels and URLs alike.
    # So a line that's nothing but badges or links has no letters left.
    residual = []
    depth = 0
    for char in text:
        if char in "([":
            depth += 1
        elif char in ")]":
            depth = max(depth - 1, 0)
        elif depth == 0:
            residual.append(char)
    return not re.search(r"[A-Za-z]", "".join(residual))


def _find_comment_start(line: str, marker: str) -> int | None:
    idx = line.find(marker)
    while idx != -1:
        if marker == "//" and (idx == 0 or line[idx - 1] != ":"):
            return idx
        if marker == "#" and line[idx + 1 : idx + 2] != "!":
            return idx
        idx = line.find(marker, idx + len(marker))
    return None


def _check_line_comment_file(path: Path, marker: str) -> list[Violation]:
    violations: list[Violation] = []
    chain: list[tuple[int, str]] = []

    def flush_chain() -> None:
        for line_no, text in chain[:-1]:
            if not ends_sentence(text):
                violations.append(Violation(path, line_no, "wrapped", text))
        chain.clear()

    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    for line_no, raw in enumerate(lines, start=1):
        stripped = raw.strip()
        is_shebang = marker == "#" and stripped.startswith("#!")
        if stripped.startswith(marker) and not is_shebang:
            comment = stripped[len(marker):].strip()
            if not comment:
                flush_chain()
                continue
            if has_multiple_sentences(comment):
                violations.append(Violation(path, line_no, "multiple", comment))
            chain.append((line_no, comment))
            continue

        flush_chain()
        idx = _find_comment_start(raw, marker)
        if idx is not None:
            comment = raw[idx + len(marker):].strip()
            if comment and has_multiple_sentences(comment):
                violations.append(Violation(path, line_no, "multiple", comment))

    flush_chain()
    return violations


def check_cpp_file(path: Path) -> list[Violation]:
    return _check_line_comment_file(path, "//")


def check_python_file(path: Path) -> list[Violation]:
    return _check_line_comment_file(path, "#")


def check_markdown_file(path: Path) -> list[Violation]:
    violations: list[Violation] = []
    chain: list[tuple[int, str]] = []
    in_fence = False

    def flush_chain() -> None:
        for line_no, text in chain[:-1]:
            if not ends_sentence(text):
                violations.append(Violation(path, line_no, "wrapped", text))
        chain.clear()

    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    for line_no, raw in enumerate(lines, start=1):
        stripped = raw.strip()

        if FENCE.match(stripped):
            in_fence = not in_fence
            flush_chain()
            continue
        if in_fence:
            continue
        if not stripped or stripped.startswith("#") or is_pure_markup(stripped):
            flush_chain()
            continue
        if LIST_MARKER.match(stripped):
            # A new list item never continues the previous line's sentence.
            flush_chain()

        text = stripped[1:].strip() if stripped.startswith(">") else stripped
        if has_multiple_sentences(text):
            violations.append(Violation(path, line_no, "multiple", text))
        chain.append((line_no, text))

    flush_chain()
    return violations


def find_violations(root: Path) -> list[Violation]:
    violations: list[Violation] = []

    for pattern in MARKDOWN_GLOBS:
        for path in sorted(root.glob(pattern)):
            violations.extend(check_markdown_file(path))
    for pattern in CPP_GLOBS:
        for path in sorted(root.glob(pattern)):
            violations.extend(check_cpp_file(path))
    for pattern in PYTHON_GLOBS:
        for path in sorted(root.glob(pattern)):
            if path.resolve() == Path(__file__).resolve():
                continue
            violations.extend(check_python_file(path))

    return violations


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=DEFAULT_ROOT,
        help="Repository root (defaults to the parent of scripts/)",
    )
    args = parser.parse_args()

    violations = find_violations(args.root)

    if violations:
        print(f"Found {len(violations)} one-sentence-per-line violation(s):\n")
        for violation in violations:
            print(violation)
            print()
        print(
            "Put each sentence on its own line -- however long -- and "
            "split any line that holds more than one."
        )
        return 1

    print(
        "OK: every checked comment and markdown line holds exactly one, "
        "unwrapped sentence."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
