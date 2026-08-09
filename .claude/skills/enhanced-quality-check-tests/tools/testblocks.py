"""Shared parsing for the enhanced-quality-check-tests tools.

Every tool walks TEST/TEST_F/TEST_P bodies, so the brace matching lives
here once.
"""
import pathlib
import re

TEST_RE = re.compile(r"^[ \t]*(?:TYPED_)?TEST(?:_F|_P)?[ \t]*\(", re.M)

SKIP_PREFIXES = ("build/", "build-coverage/")

ASSERT_MACROS = (
    "EXPECT_EQ", "EXPECT_NE", "EXPECT_TRUE", "EXPECT_FALSE", "EXPECT_LT",
    "EXPECT_LE", "EXPECT_GT", "EXPECT_GE", "EXPECT_THROW", "EXPECT_NO_THROW",
    "EXPECT_THAT", "EXPECT_NEAR", "EXPECT_DOUBLE_EQ", "EXPECT_FLOAT_EQ",
    "EXPECT_STREQ", "EXPECT_CALL",
    "ASSERT_EQ", "ASSERT_NE", "ASSERT_TRUE", "ASSERT_FALSE", "ASSERT_LT",
    "ASSERT_LE", "ASSERT_GT", "ASSERT_GE", "ASSERT_THROW", "ASSERT_NO_THROW",
    "ASSERT_THAT", "ASSERT_NEAR",
)

ASSERT_RE = re.compile(
    r"\b(EXPECT_|ASSERT_|FAIL\(|SUCCEED\(|ADD_FAILURE|GTEST_SKIP|"
    r"static_assert|expect[A-Z]\w*\s*\()")


def match_delimited(text, start, opener, closer):
    """Return the index of the closer matching the opener at `start`."""
    depth = 0
    i = start
    while i < len(text):
        if text[i] == opener:
            depth += 1
        elif text[i] == closer:
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def test_blocks(text):
    """Yield (suite, name, body, line) for each test in one file."""
    for m in TEST_RE.finditer(text):
        open_paren = text.index("(", m.start())
        close_paren = match_delimited(text, open_paren, "(", ")")
        if close_paren == -1:
            continue

        header = " ".join(text[open_paren + 1:close_paren].split())
        parts = [p.strip() for p in header.split(",")]
        if len(parts) < 2:
            continue

        brace = text.find("{", close_paren)
        if brace == -1:
            continue
        end = match_delimited(text, brace, "{", "}")
        if end == -1:
            continue

        yield (
            parts[0],
            parts[-1],
            text[brace + 1:end],
            text[:m.start()].count("\n") + 1,
        )


def test_files(root):
    """Every checked-in test source under `root`, build trees excluded."""
    root = pathlib.Path(root)
    for path in sorted(root.rglob("*Test.cpp")):
        relative = str(path.relative_to(root))
        if relative.startswith(SKIP_PREFIXES):
            continue
        yield path, relative


def macro_calls(body, names=ASSERT_MACROS):
    """Yield (macro, [arguments], offset) for assertion macros in a body."""
    pattern = re.compile(r"\b(" + "|".join(names) + r")\s*\(")
    for m in pattern.finditer(body):
        open_paren = m.end() - 1
        close_paren = match_delimited(body, open_paren, "(", ")")
        if close_paren == -1:
            continue
        yield m.group(1), split_arguments(
            body[open_paren + 1:close_paren]), m.start()


def split_arguments(text):
    """Split a macro argument list on its top-level commas."""
    out = []
    depth = 0
    current = ""
    for ch in text:
        if ch in "([{<":
            depth += 1
        elif ch in ")]}>":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(current)
            current = ""
        else:
            current += ch
    out.append(current)
    return [" ".join(a.split()) for a in out]


def root_identifier(expression):
    """The object an assertion is about: `a.b().c` -> `a`."""
    text = expression.strip().lstrip("&*(")
    m = re.match(r"[A-Za-z_]\w*", text)
    return m.group(0) if m else ""
