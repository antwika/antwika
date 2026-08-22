#!/usr/bin/env python3

import argparse
import ast
import io
import re
import sys
import tokenize
from dataclasses import dataclass
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent

CPP_GLOBS = (
    "src/**/*.cpp",
    "src/**/*.hpp",
    "backends/**/*.cpp",
    "backends/**/*.hpp",
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
YAML_GLOBS = (".github/workflows/*.yml",)
SHELL_GLOBS = ("scripts/*.sh",)
DOCKER_GLOBS = (".devcontainer/*/Dockerfile",)

DOCKER_DIRECTIVES = ("# syntax=", "# escape=")

MARKDOWN_GLOB = "**/*.md"

PERMITTED_MARKDOWN = ("README.md", "CHANGELOG.md")

MARKDOWN_DIR = "docs"

EXCLUDED_DIRS = (".claude", ".git")

STRAY_MARKDOWN = "markdown outside README.md, CHANGELOG.md and docs/"

PERMITTED_MARKERS = (
    "GCOVR_EXCL_LINE",
    "GCOVR_EXCL_START",
    "GCOVR_EXCL_STOP",
)

PHASE_MARKERS = ("Arrange", "Act", "Assert")

TEST_BODY = re.compile(r"\b(?:TYPED_)?TEST(?:_F|_P)?\s*\(")

UNFINISHED_MARKERS = ("TODO", "FIXME", "HACK", "XXX")

NOT_A_FUNCTION = re.compile(
    r"\b(namespace|struct|class|enum|union|extern)\b"
)

MAX_TEST_NAME_LENGTH = 75

TEST_MACRO = re.compile(
    r"\b(?:TYPED_)?TEST(?:_F|_P)?\s*\([^,()]+,"
    r"\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)"
)

TEST_FIXTURE = re.compile(
    r"\b(?:TYPED_)?TEST(?:_F|_P)?\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,"
)

FIXTURE_SUFFIX = "test fixture name does not end in 'Test'"

TEST_NAME_GRAMMAR = "test name is not Method_DoesX"
INCLUDE_ORDER = "include group out of order"

TEST_DOUBLE = re.compile(
    r"\b(?:class|struct)\s+((?:Mock|Fake)(?=[A-Z_])[A-Za-z0-9_]*)\b"
)

MOCK_METHOD = re.compile(r"\bMOCK_METHOD\b")

MOCK_WITHOUT_GMOCK = "'Mock' test double without MOCK_METHOD"
FAKE_WITH_GMOCK = "'Fake' test double with MOCK_METHOD"

DOUBLE_OUTSIDE_THE_TREE = (
    "library test double outside tests/mocks or tests/fakes"
)

DOUBLE_WITHOUT_A_PREFIX = "test double without a 'Mock' or 'Fake' prefix"

IMPLEMENTED_INTERFACE = re.compile(
    r"\b(?:class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)(?:\s+final)?\s*:\s*"
    r"(?:public|private|protected)\s+([A-Za-z_][A-Za-z0-9_:]*)"
)

HEADER_NAME = "header not named for the one type it declares"

CROWDED_HEADER = "header declaring more than one class or struct"

SOURCE_LIST_ORDER = "source list out of alphabetical order"

ABSTRACT_WITHOUT_I = "abstract type without the 'I' prefix"

CONSTANT_WITHOUT_K = "namespace-scope constant without the 'k' prefix"

MISSING_FINAL = "type nothing derives from is not 'final'"

HEADER_LEADS_WITH_TEST = "header name leads with 'Test'"

TYPE_HEAD = re.compile(
    r"(?<!enum )\b(?:class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)"
    r"(\s*<[^<>{};]*>)?"
    r"(\s+final\b)?"
    r"(\s*:[^;{]*)?"
    r"(\s*\{)"
)

GMOCK_WRAPPER = re.compile(
    r"\b(?:Nice|Strict|Naggy)Mock\s*<\s*([A-Za-z_][A-Za-z0-9_:]*)"
)

TYPED_SUITE = re.compile(
    r"\b(?:REGISTER_)?TYPED_TEST_SUITE(?:_P)?\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)"
)

INSTANTIATION = re.compile(
    r"\bINSTANTIATE_(?:TYPED_)?TEST_SUITE_P\s*\([^,]*,"
    r"\s*([A-Za-z_][A-Za-z0-9_]*)"
)

UNSCOPED_ENUM = "enum that is not an 'enum class'"
UNSIZED_ENUM = "enum without an underlying type"

ENUM = re.compile(
    r"\benum\b(?:\s+(class|struct))?\s+([A-Za-z_][A-Za-z0-9_]*)"
    r"\s*(:[^;{]*)?([;{])"
)

MISSING_ANNOTATION = "Python function without full type annotations"

CONSTEXPR = re.compile(r"\bconstexpr\b")

CONSTANT_NAME = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*$")

PURE_VIRTUAL = re.compile(r"\bvirtual\b[^;{}]*=\s*0\s*;")

DECLARED_TYPE = re.compile(
    r"\b(?:class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)\b[^;{]*\{"
)

SOURCE_ENTRY = re.compile(
    r"^(\s+)([A-Za-z0-9_./${}]+\.(?:cpp|hpp))$"
)

NAMESPACE_OPENER = re.compile(r"\bnamespace\b[^;{]*$")

ENUM_HEAD = re.compile(r"\benum\b")

TYPE_OPENER = re.compile(
    r"\b(?:class|struct|union|enum(?:\s+class|\s+struct)?)\s+"
    r"(?:\[\[[^\]]*\]\]\s*)?([A-Za-z_][A-Za-z0-9_]*)"
    r"(?:\s+final)?\s*(?::[^;{]*)?$"
)

PUBLISHED_DOUBLE = re.compile(
    r"^src/libs/[A-Za-z0-9_]+/tests/(mocks|fakes)/include/"
    r"antwika/[A-Za-z0-9_]+/(mocks|fakes)/"
)

TYPE_DECLARATION = re.compile(r"^(?:class|struct)\s+[A-Za-z_]")

SKIPPED_ABOVE_A_TYPE = re.compile(r"^(?:template\s*<|\[\[)")

BLOCK_ON_A_TYPE = "Doxygen block on a class or struct"

ASSERTION_MACRO = re.compile(r"\b(?:EXPECT|ASSERT)_[A-Z_]+\s*\(")

PROSE_IN_AN_ASSERTION = "string literal streamed into an assertion"

DOCSTRING = "Python docstring"

MIGRATING_RULES = frozenset()

INCLUDE = re.compile(r"^[ \t]*#[ \t]*include[ \t]+([<\"][^>\"]+[>\"])",
                     re.MULTILINE)

INCLUDE_GROUPS = (
    "own",
    "third-party",
    "std",
    "project-angled",
    "project-quoted",
)


@dataclass
class Comment:
    line: int
    text: str
    block: bool
    in_function: bool
    in_test: bool = False
    own_line: bool = False


@dataclass
class Violation:
    path: Path
    line: int
    rule: str

    def migrating(self) -> bool:
        return self.rule in MIGRATING_RULES


def _skip_raw_string(text: str, i: int) -> int:
    open_paren = text.find("(", i + 2)

    if open_paren == -1:
        return -1

    delim = text[i + 2:open_paren]

    if len(delim) > 16 or any(c in ' ()\\\t\n' for c in delim):
        return -1

    close = ")" + delim + '"'
    end = text.find(close, open_paren + 1)

    return -1 if end == -1 else end + len(close)


def _skip_literal(text: str, i: int) -> int:
    quote = text[i]
    i += 1

    while i < len(text):
        if text[i] == "\\":
            i += 2
            continue
        if text[i] == quote:
            return i + 1
        if text[i] == "\n":
            return i
        i += 1

    return i


def find_comments(text: str) -> list[Comment]:
    comments: list[Comment] = []
    stack: list[tuple[bool, bool]] = []
    statement = ""
    i = 0

    def starts_its_line(at: int) -> bool:
        start = text.rfind("\n", 0, at) + 1

        return not text[start:at].strip()

    while i < len(text):
        char = text[i]

        if char == "R" and text.startswith('R"', i):
            end = _skip_raw_string(text, i)
            if end != -1:
                i = end
                continue

        if char in "\"'":
            i = _skip_literal(text, i)
            continue

        if text.startswith("//", i):
            end = text.find("\n", i)
            end = len(text) if end == -1 else end
            comments.append(
                Comment(
                    line=text.count("\n", 0, i) + 1,
                    text=text[i:end],
                    block=False,
                    in_function=any(f for f, _ in stack),
                    in_test=any(t for _, t in stack),
                    own_line=starts_its_line(i),
                )
            )
            i = end
            continue

        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = len(text) if end == -1 else end + 2
            comments.append(
                Comment(
                    line=text.count("\n", 0, i) + 1,
                    text=text[i:end],
                    block=True,
                    in_function=any(f for f, _ in stack),
                    in_test=any(t for _, t in stack),
                    own_line=starts_its_line(i),
                )
            )
            i = end
            continue

        if char == "{":
            opens_function = (
                "(" in statement
                and ")" in statement
                and not NOT_A_FUNCTION.search(statement)
            )
            opens_test = opens_function and bool(TEST_BODY.search(statement))
            stack.append((opens_function, opens_test))
            statement = ""
            i += 1
            continue

        if char == "}":
            if stack:
                stack.pop()
            statement = ""
            i += 1
            continue

        if char == ";":
            statement = ""
            i += 1
            continue

        statement += char
        i += 1

    return comments


def mask_cpp(text: str) -> str:
    out = list(text)
    i = 0

    def blank(start: int, end: int) -> None:
        for k in range(start, min(end, len(out))):
            if out[k] != "\n":
                out[k] = " "

    while i < len(text):
        char = text[i]

        if char == "R" and text.startswith('R"', i):
            end = _skip_raw_string(text, i)
            if end != -1:
                blank(i, end)
                i = end
                continue

        if char in "\"'":
            end = _skip_literal(text, i)
            blank(i, end)
            i = end
            continue

        if text.startswith("//", i):
            end = text.find("\n", i)
            end = len(text) if end == -1 else end
            blank(i, end)
            i = end
            continue

        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = len(text) if end == -1 else end + 2
            blank(i, end)
            i = end
            continue

        i += 1

    return "".join(out)


def _class_body(text: str, start: int) -> tuple[int, int] | None:
    i = start

    while i < len(text):
        if text[i] == ";":
            return None
        if text[i] == "{":
            break
        i += 1
    else:
        return None

    depth = 0

    for k in range(i, len(text)):
        if text[k] == "{":
            depth += 1
        elif text[k] == "}":
            depth -= 1
            if depth == 0:
                return (i, k)

    return (i, len(text))


def find_test_double_violations(text: str) -> list[tuple[int, str]]:
    masked = mask_cpp(text)
    found = []

    for match in TEST_DOUBLE.finditer(masked):
        body = _class_body(masked, match.end())

        if body is None:
            continue

        name = match.group(1)
        gmock = bool(MOCK_METHOD.search(masked[body[0]:body[1]]))
        line = masked.count("\n", 0, match.start(1)) + 1

        if name.startswith("Mock") and not gmock:
            found.append((line, MOCK_WITHOUT_GMOCK))
        elif name.startswith("Fake") and gmock:
            found.append((line, FAKE_WITH_GMOCK))

    return found


def derivable_names(masked: str) -> set[str]:
    names: set[str] = set()

    names.update(TEST_FIXTURE.findall(masked))
    names.update(TYPED_SUITE.findall(masked))
    names.update(INSTANTIATION.findall(masked))

    for match in GMOCK_WRAPPER.finditer(masked):
        names.add(match.group(1).rsplit("::", 1)[-1])

    for match in TYPE_HEAD.finditer(masked):
        if match.group(4):
            names.update(re.findall(r"\b([A-Za-z_]\w*)", match.group(4)))

    return names


def find_unfinal_types(masked: str, derivable: set[str]) -> list[int]:
    found = []

    for match in TYPE_HEAD.finditer(masked):
        if match.group(3) or match.group(1) in derivable:
            continue

        found.append(masked.count("\n", 0, match.start(1)) + 1)

    return found


def find_loose_enums(text: str) -> list[tuple[int, str]]:
    masked = mask_cpp(text)
    found = []

    for match in ENUM.finditer(masked):
        if match.group(4) == ";" and not match.group(3):
            continue

        line = masked.count("\n", 0, match.start(2)) + 1

        if not match.group(1):
            found.append((line, UNSCOPED_ENUM))
        elif not match.group(3):
            found.append((line, UNSIZED_ENUM))

    return found


def find_unannotated_functions(text: str) -> list[int]:
    try:
        tree = ast.parse(text)
    except SyntaxError:
        return []

    found = []

    for node in ast.walk(tree):
        if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            continue

        arguments = [
            *node.args.posonlyargs,
            *node.args.args,
            *node.args.kwonlyargs,
        ]

        if node.args.vararg:
            arguments.append(node.args.vararg)

        if node.args.kwarg:
            arguments.append(node.args.kwarg)

        bare = [
            a
            for a in arguments
            if a.annotation is None and a.arg not in ("self", "cls")
        ]

        if node.returns is None or bare:
            found.append(node.lineno)

    return sorted(found)


def _declared_constant(statement: str) -> str | None:
    if not CONSTEXPR.search(statement):
        return None

    body = statement[statement.rindex("constexpr") + len("constexpr"):]
    depth = 0

    for i, char in enumerate(body):
        if char in "<([":
            depth += 1
        elif char in ">)]":
            depth -= 1
        elif char == "=" and depth == 0:
            body = body[:i]
            break
    else:
        if "(" in body:
            return None

    match = CONSTANT_NAME.search(body)

    return match.group(1) if match else None


def find_unprefixed_constants(text: str) -> list[int]:
    masked = mask_cpp(text)
    stack: list[str] = []
    statement = ""
    began = 0
    found = []
    i = 0

    def check() -> None:
        if any(kind != "namespace" for kind in stack):
            return

        name = _declared_constant(statement)

        if name is None or re.match(r"^k[A-Z]", name):
            return

        at = began + statement.rindex(name)
        found.append(masked.count("\n", 0, at) + 1)

    while i < len(masked):
        char = masked[i]

        if char == "{":
            check()
            kind = (
                "namespace"
                if NAMESPACE_OPENER.search(statement)
                else "block"
            )
            stack.append(kind)
            statement = ""
            began = i + 1
        elif char == "}":
            if stack:
                stack.pop()
            statement = ""
            began = i + 1
        elif char == ";":
            check()
            statement = ""
            began = i + 1
        else:
            statement += char

        i += 1

    return found


def find_abstract_types_without_i(text: str) -> list[int]:
    masked = mask_cpp(text)
    found = []

    for match in DECLARED_TYPE.finditer(masked):
        depth = 1
        i = match.end()

        while i < len(masked) and depth:
            depth += (masked[i] == "{") - (masked[i] == "}")
            i += 1

        if not PURE_VIRTUAL.search(masked[match.end():i]):
            continue

        name = match.group(1)

        if not re.match(r"^I[A-Z]", name):
            found.append(masked.count("\n", 0, match.start(1)) + 1)

    return found


def find_unsorted_source_lists(text: str) -> list[int]:
    found = []
    run: list[tuple[int, str]] = []
    indent = ""

    def close() -> None:
        if len(run) < 2:
            return

        entries = [entry for _, entry in run]

        for (line, entry), wanted in zip(run, sorted(entries)):
            if entry != wanted:
                found.append(line)
                return

    for number, raw in enumerate(text.split("\n"), start=1):
        match = SOURCE_ENTRY.match(raw)

        if match and (not run or match.group(1) == indent):
            indent = match.group(1)
            run.append((number, match.group(2)))
            continue

        close()
        run = [(number, match.group(2))] if match else []
        indent = match.group(1) if match else indent

    close()

    return found


def _without_directives(masked: str) -> str:
    kept = []

    for line in masked.split("\n"):
        kept.append("" if line.lstrip().startswith("#") else line)

    return "\n".join(kept)


def _namespace_scope(
    masked: str,
) -> tuple[list[tuple[str, int, bool]], bool]:
    types: list[tuple[str, int, bool]] = []
    stack: list[str] = []
    statement = ""
    began = 0
    other = False
    i = 0

    def outermost() -> bool:
        return all(kind == "namespace" for kind in stack)

    while i < len(masked):
        char = masked[i]

        if char == "{":
            if NAMESPACE_OPENER.search(statement):
                kind = "namespace"
            else:
                match = TYPE_OPENER.search(statement)
                kind = "type" if match else "other"

                if outermost():
                    if kind == "type":
                        at = began + match.start(1)
                        types.append(
                            (
                                match.group(1),
                                masked.count("\n", 0, at) + 1,
                                bool(ENUM_HEAD.search(match.group(0))),
                            )
                        )
                    elif statement.strip():
                        other = True

            stack.append(kind)
            statement = ""
            began = i + 1
        elif char == "}":
            if stack:
                stack.pop()
            statement = ""
            began = i + 1
        elif char == ";":
            if outermost() and statement.strip():
                other = True
            statement = ""
            began = i + 1
        else:
            statement += char

        i += 1

    return types, other


def find_misnamed_headers(text: str, stem: str) -> list[int]:
    masked = _without_directives(mask_cpp(text))
    types, other = _namespace_scope(masked)

    if other or len(types) != 1:
        return []

    name, line, _ = types[0]

    return [] if name == stem else [line]


def find_crowded_headers(text: str) -> list[int]:
    masked = _without_directives(mask_cpp(text))
    types, _ = _namespace_scope(masked)

    shapes = [(name, line) for name, line, enum in types if not enum]

    return [line for _, line in shapes[1:]]


def find_unprefixed_doubles(text: str, relative: Path) -> list[int]:
    if "/tests/" not in relative.as_posix():
        return []

    masked = mask_cpp(text)
    found = []

    for match in IMPLEMENTED_INTERFACE.finditer(masked):
        base = match.group(2).rsplit("::", 1)[-1]

        if not re.match(r"^I[A-Z]", base):
            continue

        if re.match(r"^(?:Mock|Fake)[A-Z]", match.group(1)):
            continue

        found.append(masked.count("\n", 0, match.start(1)) + 1)

    return found


def find_misplaced_doubles(text: str, relative: Path) -> list[int]:
    posix = relative.as_posix()

    if not posix.startswith("src/libs/"):
        return []

    if PUBLISHED_DOUBLE.match(posix):
        return []

    masked = mask_cpp(text)

    return [
        masked.count("\n", 0, match.start(1)) + 1
        for match in TEST_DOUBLE.finditer(masked)
    ]


def find_blocks_on_types(text: str) -> list[int]:
    masked = mask_cpp(text).split("\n")
    found = []

    for comment in find_comments(text):
        if not comment.block:
            continue

        after = comment.line + comment.text.count("\n")

        for line in masked[after:]:
            stripped = line.strip()

            if not stripped or SKIPPED_ABOVE_A_TYPE.match(stripped):
                continue

            if TYPE_DECLARATION.match(stripped):
                found.append(comment.line)

            break

    return found


def is_permitted_marker(comment: Comment) -> bool:
    if comment.block:
        return False

    body = comment.text.lstrip("/").strip()

    return body in PERMITTED_MARKERS


def is_permitted_phase_marker(comment: Comment) -> bool:
    if comment.block or not comment.in_test or not comment.own_line:
        return False

    return comment.text.lstrip("/").strip() in PHASE_MARKERS


def rules_broken(comment: Comment) -> list[str]:
    broken = []

    if is_permitted_marker(comment) or is_permitted_phase_marker(comment):
        return broken

    for marker in UNFINISHED_MARKERS:
        if marker in comment.text:
            broken.append(f"unfinished-work marker '{marker}'")
            break

    if comment.in_function:
        broken.append("comment inside a function body")

    if not comment.block:
        broken.append("'//' comment that is not a permitted tool marker")
        return broken

    if not comment.text.startswith("/**"):
        broken.append("block comment that is not a Doxygen '/**' block")
        return broken

    if "@brief" not in comment.text:
        broken.append("Doxygen block without '@brief'")

    return broken


def find_python_comments(text: str) -> list[int]:
    lines = []

    try:
        tokens = tokenize.generate_tokens(io.StringIO(text).readline)
        for token in tokens:
            if token.type != tokenize.COMMENT:
                continue
            if token.string.startswith("#!") and token.start[0] == 1:
                continue
            lines.append(token.start[0])
    except (tokenize.TokenError, IndentationError, SyntaxError):
        return []

    return lines


def find_docstrings(text: str) -> list[int]:
    lines = []
    scopes = (
        ast.Module,
        ast.ClassDef,
        ast.FunctionDef,
        ast.AsyncFunctionDef,
    )

    try:
        tree = ast.parse(text)
    except SyntaxError:
        return []

    for node in ast.walk(tree):
        if not isinstance(node, scopes):
            continue

        body = getattr(node, "body", [])

        if not body or not isinstance(body[0], ast.Expr):
            continue

        first = body[0].value

        if isinstance(first, ast.Constant) and isinstance(first.value, str):
            lines.append(body[0].lineno)

    return sorted(lines)


def find_hash_comments(text: str, multiline_strings: bool) -> list[int]:
    lines = []
    quote = ""
    i = 0

    while i < len(text):
        char = text[i]

        if quote:
            if char == "\\":
                i += 2
                continue
            if char == quote:
                quote = ""
            elif char == "\n" and not multiline_strings:
                quote = ""
            i += 1
            continue

        if char in "\"'":
            quote = char
            i += 1
            continue

        if char == "#":
            line = text.count("\n", 0, i) + 1
            start = text.rfind("\n", 0, i) + 1

            if not text[start:i].strip():
                if not (line == 1 and text.startswith("#!")):
                    lines.append(line)

            end = text.find("\n", i)
            i = len(text) if end == -1 else end
            continue

        i += 1

    return lines


def find_long_test_names(text: str) -> list[tuple[int, str]]:
    found = []

    for match in TEST_MACRO.finditer(text):
        name = match.group(1)

        if len(name) > MAX_TEST_NAME_LENGTH:
            line = text.count("\n", 0, match.start(1)) + 1
            found.append((line, name))

    return found


def _balanced(text: str, start: int) -> int:
    depth = 0

    for i in range(start, len(text)):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                return i + 1

    return -1


def _statement_end(text: str, start: int) -> int:
    depth = 0

    for i in range(start, len(text)):
        char = text[i]

        if char in "([{":
            depth += 1
        elif char in ")]}":
            if depth == 0:
                return i
            depth -= 1
        elif char == ";" and depth == 0:
            return i

    return len(text)


def find_prose_streams(text: str) -> list[int]:
    masked = mask_cpp(text)
    found = []

    for match in ASSERTION_MACRO.finditer(masked):
        close = _balanced(masked, masked.index("(", match.start()))

        if close == -1:
            continue

        end = _statement_end(masked, close)
        tail = masked[close:end]

        if "<<" not in tail:
            continue

        stream = close + tail.index("<<") + 2

        if '"' in text[stream:end]:
            found.append(masked.count("\n", 0, match.start()) + 1)

    return found


def find_ungrammatical_fixture_names(text: str) -> list[tuple[int, str]]:
    masked = mask_cpp(text)
    found = []

    for match in TEST_FIXTURE.finditer(masked):
        name = match.group(1)

        if not name.endswith("Test"):
            line = masked.count("\n", 0, match.start(1)) + 1
            found.append((line, name))

    return found


def find_ungrammatical_test_names(text: str) -> list[tuple[int, str]]:
    found = []

    for match in TEST_MACRO.finditer(text):
        name = match.group(1)

        if "_" not in name:
            line = text.count("\n", 0, match.start(1)) + 1
            found.append((line, name))

    return found


def include_group(include: str, stem: str) -> str:
    body = include[1:-1]

    if include.startswith('"'):
        if body == f"{stem}.hpp" or body.endswith(f"/{stem}.hpp"):
            return "own"
        return "project-quoted"

    if "/" not in body:
        return "std"

    if body.startswith("antwika/"):
        return "project-angled"

    return "third-party"


def find_include_order_violations(text: str, stem: str) -> list[int]:
    reached = 0
    depth = 0

    for number, line in enumerate(text.split("\n"), start=1):
        stripped = line.strip()

        if not stripped:
            continue

        if re.match(r"#\s*(if|ifdef|ifndef)\b", stripped):
            depth += 1
            continue

        if re.match(r"#\s*endif\b", stripped):
            depth = max(0, depth - 1)
            continue

        if re.match(r"#\s*(else|elif)\b", stripped):
            continue

        if depth:
            continue

        if re.match(r"#\s*pragma\s+once\b", stripped):
            continue

        match = INCLUDE.match(line)

        if not match:
            break

        rank = INCLUDE_GROUPS.index(include_group(match.group(1), stem))

        if rank < reached:
            return [number]

        reached = rank

    return []


def is_excluded(relative: Path) -> bool:
    first = relative.parts[0]

    return first in EXCLUDED_DIRS or first.startswith("build")


def find_stray_markdown(root: Path) -> list[Path]:
    stray = []

    for path in find_paths(root, MARKDOWN_GLOB):
        relative = path.relative_to(root)

        if is_excluded(relative):
            continue

        if relative.as_posix() in PERMITTED_MARKDOWN:
            continue

        if relative.parts[0] == MARKDOWN_DIR:
            continue

        stray.append(path)

    return stray


def find_violations(root: Path) -> list[Violation]:
    violations: list[Violation] = []

    sources = {
        path: path.read_text(encoding="utf-8", errors="ignore")
        for pattern in CPP_GLOBS
        for path in find_paths(root, pattern)
    }

    derivable: set[str] = set()

    for text in sources.values():
        derivable |= derivable_names(mask_cpp(text))

    for pattern in CPP_GLOBS:
        for path in find_paths(root, pattern):
            text = sources[path]

            if path.suffix == ".hpp" and re.match(r"^Test[A-Z]", path.stem):
                violations.append(
                    Violation(path, 1, HEADER_LEADS_WITH_TEST)
                )

            for line in find_unfinal_types(mask_cpp(text), derivable):
                violations.append(Violation(path, line, MISSING_FINAL))

            for comment in find_comments(text):
                for rule in rules_broken(comment):
                    violations.append(Violation(path, comment.line, rule))

            for line, name in find_long_test_names(text):
                violations.append(
                    Violation(
                        path,
                        line,
                        f"test name of {len(name)} chars, over "
                        f"{MAX_TEST_NAME_LENGTH}",
                    )
                )

            for line, _ in find_ungrammatical_test_names(text):
                violations.append(
                    Violation(path, line, TEST_NAME_GRAMMAR)
                )

            for line in find_prose_streams(text):
                violations.append(
                    Violation(path, line, PROSE_IN_AN_ASSERTION)
                )

            for line, _ in find_ungrammatical_fixture_names(text):
                violations.append(Violation(path, line, FIXTURE_SUFFIX))

            for line, rule in find_loose_enums(text):
                violations.append(Violation(path, line, rule))

            for line in find_unprefixed_constants(text):
                violations.append(
                    Violation(path, line, CONSTANT_WITHOUT_K)
                )

            for line in find_abstract_types_without_i(text):
                violations.append(
                    Violation(path, line, ABSTRACT_WITHOUT_I)
                )

            for line in find_blocks_on_types(text):
                violations.append(Violation(path, line, BLOCK_ON_A_TYPE))

            for line, rule in find_test_double_violations(text):
                violations.append(Violation(path, line, rule))

            relative = path.relative_to(root)

            if path.suffix == ".hpp":
                for line in find_misnamed_headers(text, path.stem):
                    violations.append(Violation(path, line, HEADER_NAME))

                for line in find_crowded_headers(text):
                    violations.append(
                        Violation(path, line, CROWDED_HEADER)
                    )

            for line in find_unprefixed_doubles(text, relative):
                violations.append(
                    Violation(path, line, DOUBLE_WITHOUT_A_PREFIX)
                )

            for line in find_misplaced_doubles(text, relative):
                violations.append(
                    Violation(path, line, DOUBLE_OUTSIDE_THE_TREE)
                )

            for line in find_include_order_violations(text, path.stem):
                violations.append(Violation(path, line, INCLUDE_ORDER))

    for pattern in PYTHON_GLOBS:
        for path in find_paths(root, pattern):
            text = path.read_text(encoding="utf-8", errors="ignore")
            for line in find_python_comments(text):
                violations.append(Violation(path, line, "'#' comment"))

            for line in find_docstrings(text):
                violations.append(Violation(path, line, DOCSTRING))

            for line in find_unannotated_functions(text):
                violations.append(
                    Violation(path, line, MISSING_ANNOTATION)
                )

    for path in find_stray_markdown(root):
        violations.append(Violation(path, 1, STRAY_MARKDOWN))

    globs = CMAKE_GLOBS + YAML_GLOBS + SHELL_GLOBS + DOCKER_GLOBS

    for pattern in globs:
        for path in find_paths(root, pattern):
            text = path.read_text(encoding="utf-8", errors="ignore")
            multiline = pattern in CMAKE_GLOBS
            lines = text.split("\n")

            for line in find_hash_comments(text, multiline):
                body = lines[line - 1].strip()

                if any(body.startswith(d) for d in DOCKER_DIRECTIVES):
                    continue

                violations.append(Violation(path, line, "'#' comment"))

            if pattern in CMAKE_GLOBS:
                for line in find_unsorted_source_lists(text):
                    violations.append(
                        Violation(path, line, SOURCE_LIST_ORDER)
                    )

    return violations



def find_paths(root: Path, pattern: str) -> list:
    return sorted(
        path
        for path in root.glob(pattern)
        if not any(
            part.startswith("build")
            for part in path.relative_to(root).parts[:-1]
        )
    )

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--root",
        type=Path,
        default=DEFAULT_ROOT,
        help="Repository root (defaults to the parent of scripts/)",
    )
    parser.add_argument(
        "--warn-only",
        action="store_true",
        help="Report violations without failing, during a migration",
    )
    args = parser.parse_args()

    violations = find_violations(args.root)

    migrating = [v for v in violations if v.migrating()]
    failing = [v for v in violations if not v.migrating()]

    if migrating:
        rules = sorted({v.rule for v in migrating})
        print(f"{len(migrating)} site(s) awaiting migration:")
        for rule in rules:
            count = sum(1 for v in migrating if v.rule == rule)
            print(f"  {rule}: {count}")
        print()

    if failing:
        print(f"Found {len(failing)} comment-style violation(s):")
        print()
        for violation in failing:
            print(f"{violation.path}:{violation.line}: {violation.rule}")
        print()
        print("See docs/STYLE_GUIDE.md.")

        if args.warn_only:
            print("Warning only: a migration is in progress.")
            return 0

        return 1

    print("OK: every comment follows docs/STYLE_GUIDE.md.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
