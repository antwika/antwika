#!/usr/bin/env python3

import argparse
import re
import sys
from collections.abc import Iterator
from dataclasses import dataclass, replace
from pathlib import Path

from repofiles import DEFAULT_ROOT, CPP_GLOBS, find_paths


NOT_LOWER_CAMEL = "not lowerCamelCase"

CONSTANT_SPELLING = "constant is not k followed by PascalCase"

INDEX_NAME = "index name"

ACCUMULATOR = "accumulator named out"

CROWDED_SCOPE = "one of several of its type in one scope"

CONCEPT_PLURAL = "container named for a concept"

ROLE_NAME = "parameter named for its role"

NO_TYPE_WORD = "does not carry the type"

MATH_QUANTITY = "vector or matrix not named for the quantity it holds"

PART_OF_SPEECH = "a preposition, a bare adjective or a participle"

QUERY_FORM = "query not named get, create, to, of or for"

COMMAND_GETTER = "command prefixed with get"

FACTORY_VERB = "factory not named create"

WEAK_VERB = "verb naming a category, not an effect"

PREDICATE_FORM = "predicate that is not a question"

KIND_ORDER = (
    NOT_LOWER_CAMEL,
    CONSTANT_SPELLING,
    PART_OF_SPEECH,
    QUERY_FORM,
    COMMAND_GETTER,
    FACTORY_VERB,
    WEAK_VERB,
    PREDICATE_FORM,
    MATH_QUANTITY,
    INDEX_NAME,
    ACCUMULATOR,
    CROWDED_SCOPE,
    CONCEPT_PLURAL,
    ROLE_NAME,
    NO_TYPE_WORD,
)

MATH_TYPES = frozenset({"Mat3", "Mat4", "Quat", "Vec2", "Vec3", "Vec4"})

QUANTITIES = frozenset(
    {
        "angle",
        "axis",
        "center",
        "centre",
        "color",
        "colour",
        "coordinate",
        "corner",
        "delta",
        "depth",
        "direction",
        "distance",
        "extent",
        "height",
        "matrix",
        "normal",
        "offset",
        "origin",
        "point",
        "position",
        "radius",
        "rotation",
        "scale",
        "size",
        "transform",
        "translation",
        "vector",
        "velocity",
        "width",
    }
)

PREPOSITIONS = frozenset(
    """about above across after against along amid among around at away back
    before behind below beneath beside besides between beyond by down during
    except for from here in inside into near of off on onto out outside over
    past round through throughout to toward towards under underneath until up
    upon with within without now then there again""".split()
)

IRREGULAR_PARTICIPLES = frozenset(
    """beaten begun bent blown born bought broken brought built burnt caught
    chosen done drawn driven drunk eaten fallen fed felt fought found frozen
    given gone grown heard held hidden hung kept known laid lain led lent lit
    lost made meant met paid read ridden risen said seen sent sewn shaken
    shone shot shown shut slept sold sought sown spent spoken stood stolen
    struck sung sunk swept swum taken taught thrown told torn understood woken
    won worn written""".split()
)

NOT_PARTICIPLES = frozenset(
    """ahead behind bed bend bind bled blend board bond bound bred breed
    chord cord creed dead deed embed end exceed field find fled friend greed
    grind ground head hound hundred indeed instead kind lead legend lord mind
    mound need pound proceed read record red rind round sacred second seed send
    shed shield sound speed sped spread succeed sword thousand thread weed
    wind word world wound yield""".split()
)

BARE_ADJECTIVES = frozenset(
    """big bright busy clean dark dirty dry easy empty far fast flat free full
    hard heavy last live long loud new next old own quiet ready raw same sharp
    short slow small soft still thick thin tall wet whole wide""".split()
)

INDEX_NAMES = frozenset(
    {"at", "row", "column", "rank", "index", "i", "j", "k", "n"}
)

REQUIRED_NAMES = frozenset(
    """begin cbegin cend data end hash rbegin rend swap""".split()
)

BUILTIN_TYPES = frozenset(
    {
        "bool",
        "char",
        "char8_t",
        "char16_t",
        "char32_t",
        "double",
        "float",
        "int",
        "long",
        "short",
        "signed",
        "size_t",
        "unsigned",
        "void",
        "wchar_t",
    }
)

WRAPPERS = {
    "array": True,
    "deque": True,
    "initializer_list": True,
    "list": True,
    "map": True,
    "multiset": True,
    "set": True,
    "span": True,
    "unordered_map": True,
    "unordered_set": True,
    "vector": True,
    "optional": False,
    "reference_wrapper": False,
    "shared_ptr": False,
    "unique_ptr": False,
    "weak_ptr": False,
}

EXTRA_NAMESPACES = frozenset(
    {
        "antwika",
        "conformance",
        "decordetail",
        "detail",
        "events",
        "fakes",
        "mapfile",
        "mocks",
        "support",
        "voxeldetail",
        "weavedetail",
    }
)

SPECIFIERS = (
    r"(?:static|inline|constexpr|consteval|constinit|const|volatile"
    r"|mutable|thread_local|extern|typename|struct|class|enum|register)\b"
)

QUALIFIED = r"(?:::)?(?:[A-Za-z_]\w*\s*::\s*)*[A-Za-z_]\w*"

TEMPLATE_ARGS = r"(?:\s*<[^;{}]*>)?"

DECLARATOR = re.compile(
    r"^(?:\s*" + SPECIFIERS + r")*\s*"
    r"(?P<type>" + QUALIFIED + TEMPLATE_ARGS + r")"
    r"(?:\s*" + SPECIFIERS + r")*"
    r"(?:\s*[*&]+\s*|\s+)"
    r"(?P<name>[A-Za-z_]\w*)\s*"
    r"(?:\[[^\]]*\]\s*)*"
    r"(?P<init>=[^=]|\{|\(|$)",
    re.S,
)

PARAMETER = re.compile(
    r"^\s*(?:\[\[[^\]]*\]\]\s*)*"
    r"(?P<type>.*?)"
    r"(?:\s*[*&]+\s*|\s+)"
    r"(?P<name>[A-Za-z_]\w*)\s*"
    r"(?:\[[^\]]*\]\s*)*"
    r"(?:=[^=].*)?$",
    re.S,
)

FUNCTION_POINTER = re.compile(r"\(\s*\*+\s*(?P<name>[A-Za-z_]\w*)\s*\)\s*\(")

DECLARED_TYPE = re.compile(
    r"\b(?:class|struct|enum\s+class|enum\s+struct)\s+"
    r"(?:\[\[[^\]]*\]\]\s*)?([A-Z]\w*)"
    r"|\busing\s+([A-Z]\w*)\s*="
    r"|\busing\s+[A-Za-z_][\w:]*::([A-Z]\w*)\s*;"
)

CONSTANT_NAME = re.compile(r"\b([A-Za-z_]\w*)\s*$")

LOWER_CAMEL = re.compile(r"^[a-z][A-Za-z0-9]*$")

GET_PREFIXED = re.compile(r"^get[A-Z0-9]")

QUERY_PREFIXED = re.compile(r"^(?:get|create|to)[A-Z0-9]")

LOOKUP_SUFFIXED = re.compile(r"[a-z0-9](?:Of|For|At|In|From|Along)$")

FACTORY_PREFIXED = re.compile(r"^(?:make|build)[A-Z0-9]")

WEAK_PREFIXED = re.compile(r"^(?:handle|process|manage|do|perform)[A-Z0-9]")

QUESTION_PREFIXED = re.compile(r"^(?:is|has|was|were|can|should)[A-Z0-9]")

WRITTEN_THROUGH = re.compile(r"[*&]")

ATTRIBUTE = re.compile(r"\[\[[^\]]*\]\]")

QUALIFIER_CHAIN = re.compile(r"(?:[A-Za-z_]\w*\s*::\s*)+$")

ACCESS_SPECIFIER = re.compile(r"^\s*(?:public|private|protected)\s*:\s*")

TRAILING_RETURN = re.compile(r"->\s*(?P<type>[^{;]+)")

CONSTANT_SPELLED = re.compile(r"^k[A-Z][A-Za-z0-9]*$")

NAMESPACE_OPENER = re.compile(r"\bnamespace\b[^;{]*$")

ENUM_OPENER = re.compile(r"\benum\b")

TYPE_OPENER = re.compile(
    r"\b(?:class|struct|union)\s+"
    r"(?:\[\[[^\]]*\]\]\s*)?([A-Za-z_]\w*)"
    r"(?:\s+final)?\s*(?::[^;{]*)?$"
)

RANGE_FOR = re.compile(
    r"\bfor\s*\(\s*(?P<declaration>[^;()]*?)\s*:\s*[^{;]*\)"
)

FOR_INIT = re.compile(r"\bfor\s*\(\s*(?P<declaration>[^;()]*?)\s*;")

CONDITION_INIT = re.compile(
    r"\b(?:if|switch)\s*\(\s*(?P<declaration>[^;{}]*?)\s*;"
)

BOUND_NAMES = re.compile(
    r"\bauto\b\s*[&*]*\s*\[\s*(?P<names>[A-Za-z_]\w*"
    r"(?:\s*,\s*[A-Za-z_]\w*)*)\s*\]"
)

LAMBDA_HEAD = re.compile(r"\[[^\]\[]*\]\s*\((?P<parameters>[^()]*)\)")

LAMBDA_TAIL = re.compile(
    r"\[[^\]\[]*\]\s*(?:\([^)]*\))?\s*"
    r"(?:mutable\b|noexcept\b|->[^{]*)?\s*$"
)

STRUCTURED_BINDING = re.compile(r"\bauto\b\s*[&*]*\s*\[")

MACRO_CALL = re.compile(r"^\s*[A-Z][A-Z0-9_]{2,}\s*\(")

DIRECTIVE = re.compile(r"^[ \t]*#.*$", re.MULTILINE)

BLOCK_HEADS = frozenset(
    {
        "break",
        "case",
        "catch",
        "co_await",
        "co_return",
        "co_yield",
        "continue",
        "default",
        "delete",
        "do",
        "else",
        "for",
        "goto",
        "if",
        "new",
        "return",
        "switch",
        "throw",
        "try",
        "while",
    }
)

REJECTED_HEADS = frozenset(
    {
        "concept",
        "friend",
        "namespace",
        "private",
        "protected",
        "public",
        "requires",
        "static_assert",
        "template",
        "typedef",
        "using",
    }
)

SPECIFIER_WORDS = frozenset(
    {
        "class",
        "const",
        "constexpr",
        "enum",
        "inline",
        "long",
        "mutable",
        "short",
        "signed",
        "static",
        "struct",
        "typename",
        "unsigned",
        "volatile",
    }
)

NOT_DECLARATORS = frozenset(
    {
        "alignof",
        "catch",
        "const_cast",
        "decltype",
        "dynamic_cast",
        "for",
        "if",
        "noexcept",
        "reinterpret_cast",
        "requires",
        "return",
        "sizeof",
        "static_cast",
        "switch",
        "typeid",
        "while",
    }
)


@dataclass(frozen=True)
class Violation:
    path: Path
    line: int
    column: int
    name: str
    kind: str
    spelled: str
    where: str = ""

    def wanted(self) -> str:
        said = f"{self.kind} ({self.where})" if self.where else self.kind

        if not self.spelled:
            return said

        return f"{said}, wanting a word of {self.spelled}"


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


def _blank_directives(masked: str) -> str:
    return DIRECTIVE.sub(lambda m: " " * len(m.group(0)), masked)


def _balanced(text: str, start: int, opener: str = "(") -> int:
    closer = {"(": ")", "{": "}", "[": "]"}[opener]
    depth = 0

    for i in range(start, len(text)):
        if text[i] == opener:
            depth += 1
        elif text[i] == closer:
            depth -= 1
            if depth == 0:
                return i + 1

    return -1


def _blank_init_lists(masked: str) -> str:
    out = list(masked)
    i = 0

    while i < len(masked):
        if masked[i] != ":":
            i += 1
            continue

        if masked[i:i + 2] == "::" or (i and masked[i - 1] == ":"):
            i += 2
            continue

        before = masked.rfind(")", 0, i)

        if before == -1 or masked[before + 1:i].strip() not in (
            "",
            "const",
            "noexcept",
            "const noexcept",
        ):
            i += 1
            continue

        end = _skip_init_list(masked, i)

        if end == -1:
            i += 1
            continue

        for k in range(i, end):
            if out[k] != "\n":
                out[k] = " "

        i = end

    return "".join(out)


def _skip_init_list(masked: str, start: int) -> int:
    i = start + 1

    while i < len(masked):
        char = masked[i]

        if char == "(":
            jumped = _balanced(masked, i, "(")
            if jumped == -1:
                return -1
            i = jumped
            continue

        if char == "{":
            jumped = _balanced(masked, i, "{")
            if jumped == -1:
                return -1

            rest = masked[jumped:]
            nxt = rest[len(rest) - len(rest.lstrip()):len(rest) - len(
                rest.lstrip()) + 1]

            if nxt == ",":
                i = jumped
                continue

            return i

        if char == ";":
            return -1

        i += 1

    return -1


def _head(statement: str) -> str:
    match = re.match(r"\s*([A-Za-z_]\w*)", statement)

    return match.group(1) if match else ""


def _opens(statement: str) -> str:
    stripped = statement.strip()

    if NAMESPACE_OPENER.search(stripped):
        return "namespace"

    head = _head(stripped)

    if head == "extern" and not stripped[len("extern"):].strip():
        return "namespace"

    if ENUM_OPENER.search(stripped):
        return "enum"

    if TYPE_OPENER.search(stripped):
        return "class"

    if head in BLOCK_HEADS:
        return "block"

    if LAMBDA_TAIL.search(stripped):
        return "function"

    if _declarator_parens(stripped) is not None:
        return "function"

    return "block"


def _declarator_parens(statement: str) -> tuple[int, int, str] | None:
    depth = 0
    found = None

    for i, char in enumerate(statement):
        if char == "(":
            if depth == 0:
                name = CONSTANT_NAME.search(statement[:i])
                if name is not None:
                    found = (i, name.group(1))
            depth += 1
        elif char == ")":
            depth = max(0, depth - 1)

    if found is None:
        return None

    close = _balanced(statement, found[0], "(")

    if close == -1:
        return None

    return (found[0], close - 1, found[1])


def _split_arguments(text: str) -> list[tuple[int, str]]:
    parts = []
    depth = 0
    began = 0

    for i, char in enumerate(text):
        if char in "([{<":
            depth += 1
        elif char in ")]}>":
            depth = max(0, depth - 1)
        elif char == "," and depth == 0:
            parts.append((began, text[began:i]))
            began = i + 1

    parts.append((began, text[began:]))

    return [(at, part) for at, part in parts if part.strip()]


def _walk(
    masked: str,
) -> Iterator[tuple[tuple[str, ...], int, str, str, str | None]]:
    stack: list[str] = []
    began = 0
    depth = 0
    i = 0

    while i < len(masked):
        char = masked[i]

        if char in "([":
            depth += 1
        elif char in ")]":
            depth = max(0, depth - 1)
        elif depth == 0 and char == "{":
            statement = masked[began:i]
            kind = _opens(statement)

            yield tuple(stack), began, statement, "{", kind

            stack.append(kind)
            began = i + 1
        elif depth == 0 and char == "}":
            if stack:
                stack.pop()
            began = i + 1
        elif depth == 0 and char == ";":
            yield tuple(stack), began, masked[began:i], ";", None

            began = i + 1

        i += 1


def _where(masked: str, at: int) -> tuple[int, int]:
    line = masked.count("\n", 0, at) + 1
    column = at - (masked.rfind("\n", 0, at) + 1) + 1

    return line, column


def project_namespaces(root: Path) -> frozenset[str]:
    names = set(EXTRA_NAMESPACES)

    for holder in ("src/libs", "src/apps"):
        directory = root / holder

        if not directory.is_dir():
            continue

        names |= {one.name for one in directory.iterdir() if one.is_dir()}

    return frozenset(names)


def declared_types(sources: dict[Path, str]) -> frozenset[str]:
    names = set()

    for masked in sources.values():
        for match in DECLARED_TYPE.finditer(masked):
            names |= {one for one in match.groups() if one}

    return frozenset(names)


def is_project_type(
    spelled: str, declared: frozenset[str], namespaces: frozenset[str]
) -> bool:
    core = spelled.strip()

    if core.startswith("::"):
        return False

    if "<" in core:
        core = core[:core.index("<")]

    if "::" in core:
        head = core.split("::", 1)[0].strip()
        last = core.rsplit("::", 1)[-1].strip()

        return head in namespaces and last[:1].isupper()

    return core in declared and core[:1].isupper()


def unwrap(spelled: str) -> tuple[str, bool]:
    plural = False
    core = _bare(spelled)

    while True:
        match = re.match(r"^(?:std::)?(\w+)\s*<(.*)>$", core, re.S)

        if match is None or match.group(1) not in WRAPPERS:
            break

        plural = plural or WRAPPERS[match.group(1)]
        arguments = _split_arguments(match.group(2))

        if not arguments:
            break

        chosen = arguments[0][1]

        if match.group(1).endswith("map") and len(arguments) > 1:
            chosen = arguments[1][1]

        core = _bare(chosen)

    return core, plural


def _bare(spelled: str) -> str:
    core = spelled.strip()

    for specifier in ("const ", "volatile ", "struct ", "class ", "typename "):
        while core.startswith(specifier):
            core = core[len(specifier):].strip()

    return core.rstrip("*& \t\n").strip()


def words(core: str) -> list[str]:
    last = core.rsplit("::", 1)[-1]
    last = re.sub(r"^I(?=[A-Z])", "", last)
    parts = re.findall(r"[A-Z][a-z]*|[A-Z]+(?![a-z])|^[a-z]+|[0-9]+", last)

    return [one.lower() for one in parts]


def _plural(word: str) -> str:
    if word.endswith(("ex", "ix")):
        return word[:-2] + "ices"

    if word.endswith(("s", "x", "z", "ch", "sh")):
        return word + "es"

    if word.endswith("y") and word[-2:-1] not in "aeiou":
        return word[:-1] + "ies"

    return word + "s"


def accepted(core: str, plural: bool) -> set[str]:
    spelled = words(core)
    whole = "".join(spelled)

    if len(whole) < 3:
        return set()

    out = {one for one in spelled if len(one) >= 3}

    out.add(whole)

    if plural:
        out |= {_plural(one) for one in set(out)}

    return out


def is_math_type(core: str) -> bool:
    return core.rsplit("::", 1)[-1] in MATH_TYPES


def carries_type(name: str, spelled: str) -> bool:
    core, plural = unwrap(spelled)
    wanted = accepted(core, plural)

    if is_math_type(core):
        wanted = wanted | QUANTITIES

    if not wanted:
        return True

    lowered = name.lower()

    return any(one in lowered for one in wanted)


def sole_word(name: str) -> str | None:
    parts = re.findall(r"[A-Z]?[a-z0-9]+|[A-Z]+(?![a-z])", name)

    return parts[0].lower() if len(parts) == 1 else None


def is_participle(word: str) -> bool:
    if word in NOT_PARTICIPLES:
        return False

    return word in IRREGULAR_PARTICIPLES or (
        word.endswith("ed") and len(word) > 3
    )


def names_a_thing(name: str, spelled: str) -> bool:
    word = sole_word(name)

    if word is None:
        return True

    if spelled.replace("const", "").strip() == "bool":
        return True

    return not (
        word in PREPOSITIONS
        or is_participle(word)
        or word in BARE_ADJECTIVES
    )


@dataclass(frozen=True)
class Site:
    path: Path
    offset: int
    name: str
    spelled: str
    scope: int
    where: str
    line: int = 0
    column: int = 0
    const_qualified: bool = False
    changes_nothing: bool = False
    abi_symbol: bool = False


def _reject(statement: str) -> bool:
    head = _head(statement)

    if re.search(r"\boperator\b", statement):
        return True

    if head in REJECTED_HEADS or head in BLOCK_HEADS:
        return True

    if MACRO_CALL.match(statement):
        return True

    if STRUCTURED_BINDING.search(statement):
        return True

    return False


def _member_site(
    path: Path, began: int, statement: str, scope: int
) -> Site | None:
    if _reject(statement):
        return None

    pointer = FUNCTION_POINTER.search(statement)

    if pointer is not None:
        return None

    if _declarator_parens(statement) is not None:
        return None

    return _declared_site(path, began, statement, scope, "member")


def _returned(lead: str) -> str:
    core = ATTRIBUTE.sub(" ", lead)

    while True:
        shorter = re.sub(r"(?:^|\s)" + SPECIFIERS + r"\s*", " ", core)

        if shorter == core:
            break

        core = shorter

    return re.sub(r"\bvirtual\b|\bfriend\b|\bexplicit\b", " ", core).strip()


def _reads_only(arguments: str) -> bool:
    for _, chunk in _split_arguments(arguments):
        if not WRITTEN_THROUGH.search(chunk):
            continue

        through = chunk[:WRITTEN_THROUGH.search(chunk).start()]

        if "const" not in through:
            return False

    return True


def _function_site(
    path: Path, began: int, statement: str, scope: int, member: bool
) -> Site | None:
    opening = ACCESS_SPECIFIER.match(statement)

    if opening is not None:
        began += opening.end()
        statement = statement[opening.end():]
        member = True

    if _reject(statement):
        return None

    if FUNCTION_POINTER.search(statement) is not None:
        return None

    parens = _declarator_parens(statement)

    if parens is None:
        return None

    open_at, close_at, name = parens

    if name in NOT_DECLARATORS or MACRO_CALL.match(statement):
        return None

    if not LOWER_CAMEL.match(name):
        return None

    head = CONSTANT_NAME.search(statement[:open_at])

    if head is None:
        return None

    before = statement[:head.start(1)]

    if _first_assignment(before) < len(before):
        return None

    lead = QUALIFIER_CHAIN.sub("", before)
    spelled = _returned(lead)

    defined_out_of_line = lead != before

    if defined_out_of_line:
        return None

    if re.search(r"\bstatic\b", before):
        member = False

    if not spelled:
        return None

    tail = statement[close_at + 1:]
    trailing = TRAILING_RETURN.search(tail)

    if trailing is not None:
        spelled = trailing.group("type").strip()

    const_qualified = re.search(r"\bconst\b", tail.split("->")[0]) is not None

    return Site(
        path,
        began + head.start(1),
        name,
        spelled,
        scope,
        "function",
        const_qualified=const_qualified,
        changes_nothing=(
            (const_qualified or not member)
            and _reads_only(statement[open_at + 1:close_at])
        ),
        abi_symbol=re.match(r"^\s*extern\b", statement) is not None,
    )


def _local_site(
    path: Path, began: int, statement: str, scope: int
) -> Site | None:
    if _reject(statement):
        return None

    if FUNCTION_POINTER.search(statement) is not None:
        return None

    parens = _declarator_parens(statement)

    if parens is not None and parens[2] in NOT_DECLARATORS:
        return None

    return _declared_site(path, began, statement, scope, "local")


def _declared_site(
    path: Path, began: int, statement: str, scope: int, where: str
) -> Site | None:
    match = DECLARATOR.match(statement)

    if match is None:
        return None

    lead = statement[:match.start("name")]

    if "." in lead or "->" in lead or "=" in lead:
        return None

    if match.group("name") in ("final", "override", "const", "noexcept"):
        return None

    return Site(
        path,
        began + match.start("name"),
        match.group("name"),
        match.group("type"),
        scope,
        where,
    )


def _parameter_sites(
    path: Path, began: int, statement: str, scope: int
) -> list[Site]:
    parens = _declarator_parens(statement)

    if parens is None:
        return []

    open_at, close_at, name = parens

    if name in NOT_DECLARATORS or name.startswith("operator"):
        return []

    if name.isupper() or MACRO_CALL.match(statement):
        return []

    found = []

    for offset, chunk in _split_arguments(statement[open_at + 1:close_at]):
        if FUNCTION_POINTER.search(chunk) is not None:
            continue

        if _unnamed(chunk):
            continue

        match = PARAMETER.match(chunk)

        if match is None:
            continue

        found.append(
            Site(
                path,
                began + open_at + 1 + offset + match.start("name"),
                match.group("name"),
                match.group("type"),
                scope,
                "parameter",
            )
        )

    return found


def _unnamed(chunk: str) -> bool:
    core = chunk.strip()

    if "=" in core:
        core = core[:core.index("=")].strip()

    core = re.sub(r"\[[^\]]*\]\s*$", "", core).strip()

    if not core or core.endswith(("*", "&", ">", "]", "...")):
        return True

    while True:
        shorter = re.sub(r"<[^<>]*>", " ", core)

        if shorter == core:
            break

        core = shorter

    tokens = [
        one
        for one in re.split(r"[\s*&]+", core)
        if one and one not in SPECIFIER_WORDS
    ]

    return len(tokens) < 2


def _constant_site(
    path: Path, began: int, statement: str
) -> Site | None:
    if "constexpr" not in statement or _reject(statement):
        return None

    if _declarator_parens(statement) is not None:
        return None

    after = statement.rsplit("constexpr", 1)[-1]
    head = began + len(statement) - len(after)
    cut = _first_assignment(after)
    name = CONSTANT_NAME.search(after[:cut])

    if name is None:
        return None

    return Site(
        path,
        head + name.start(1),
        name.group(1),
        after[:name.start(1)],
        0,
        "constant",
    )


def _first_assignment(text: str) -> int:
    depth = 0

    for i, char in enumerate(text):
        if char in "([{<":
            depth += 1
        elif char in ")]}>":
            depth = max(0, depth - 1)
        elif char == "=" and depth == 0 and text[i:i + 2] != "==":
            return i

    return len(text)


def _named_in(declaration: str) -> tuple[str, str] | None:
    match = PARAMETER.match(declaration.strip())

    if match is None:
        return None

    name = match.group("name")
    spelled = match.group("type").strip()

    if not name or not spelled or name in BLOCK_HEADS:
        return None

    return name, spelled


def stepped_over(path: Path, masked: str) -> list[Site]:
    found = []

    for pattern in (RANGE_FOR, FOR_INIT, CONDITION_INIT):
        for hit in pattern.finditer(masked):
            named = _named_in(hit.group("declaration"))

            if named is None:
                continue

            found.append(
                Site(
                    path,
                    hit.start("declaration"),
                    named[0],
                    named[1],
                    0,
                    "local",
                )
            )

    for hit in BOUND_NAMES.finditer(masked):
        for name in hit.group("names").split(","):
            found.append(
                Site(path, hit.start("names"), name.strip(), "auto", 0, "local")
            )

    for hit in LAMBDA_HEAD.finditer(masked):
        for chunk in hit.group("parameters").split(","):
            named = _named_in(chunk)

            if named is not None:
                found.append(
                    Site(
                        path,
                        hit.start("parameters"),
                        named[0],
                        named[1],
                        0,
                        "parameter",
                    )
                )

    return found


def sites_in(path: Path, masked: str) -> list[Site]:
    found: list[Site] = []
    scope = 0

    for stack, began, statement, closer, kind in _walk(masked):
        scope += 1

        if "enum" in stack:
            continue

        if closer == "{":
            if kind == "function":
                found.extend(_parameter_sites(path, began, statement, scope))

                if "function" not in stack:
                    defined = _function_site(
                        path,
                        began,
                        statement,
                        scope,
                        bool(stack) and stack[-1] == "class",
                    )

                    if defined is not None:
                        found.append(defined)

                continue

            if kind != "block":
                continue

        braced = closer == "{"

        if not stack or all(one == "namespace" for one in stack):
            constant = _constant_site(path, began, statement)

            if constant is not None:
                found.append(constant)
                continue

            if not braced:
                found.extend(
                    _parameter_sites(path, began, statement, scope)
                )

                declared = _function_site(
                    path, began, statement, scope, False
                )

                if declared is not None:
                    found.append(declared)

            continue

        if stack[-1] == "class":
            if not braced:
                found.extend(
                    _parameter_sites(path, began, statement, scope)
                )

                declared = _function_site(
                    path, began, statement, scope, True
                )

                if declared is not None:
                    found.append(declared)

            member = _member_site(path, began, statement, scope)

            if member is not None:
                found.append(member)

            continue

        if "function" in stack:
            local = _local_site(path, began, statement, scope)

            if local is not None:
                found.append(local)

    return [
        replace(site, line=where[0], column=where[1])
        for site, where in ((one, _where(masked, one.offset)) for one in found)
    ]


def _kind_of(
    site: Site, crowded: bool, plural: bool, math: bool
) -> str:
    if math:
        return MATH_QUANTITY

    if site.where == "constant":
        return NO_TYPE_WORD

    if site.name in INDEX_NAMES:
        return INDEX_NAME

    if site.name == "out" or re.match(r"^out[A-Z]", site.name):
        return ACCUMULATOR

    if crowded:
        return CROWDED_SCOPE

    if plural and site.name.endswith("s"):
        return CONCEPT_PLURAL

    if site.where == "parameter":
        return ROLE_NAME

    return NO_TYPE_WORD


def _returns(spelled: str) -> str:
    core = spelled.replace("const", " ").replace("&", " ")
    core = core.replace("*", " ").strip()

    return re.sub(r"\s+", " ", core)


def _asks_a_question(name: str) -> bool:
    if QUESTION_PREFIXED.match(name):
        return True

    first = re.match(r"[a-z0-9]+", name)

    return first is not None and first.group(0).endswith("s")


def _function_kind(site: Site) -> str | None:
    if site.name in REQUIRED_NAMES or site.abi_symbol:
        return None

    if FACTORY_PREFIXED.match(site.name):
        return FACTORY_VERB

    if WEAK_PREFIXED.match(site.name):
        return WEAK_VERB

    spelled = _returns(site.spelled)

    if spelled in ("void", "auto", ""):
        if GET_PREFIXED.match(site.name):
            return COMMAND_GETTER

        return None

    if not site.changes_nothing:
        if GET_PREFIXED.match(site.name):
            return COMMAND_GETTER

        return None

    if spelled == "bool":
        if not _asks_a_question(site.name):
            return PREDICATE_FORM

        return None

    if QUERY_PREFIXED.match(site.name) or LOOKUP_SUFFIXED.search(site.name):
        return None

    return QUERY_FORM


def judge_functions(sites: list[Site]) -> list[Violation]:
    found = []
    asking = {
        site.name
        for site in sites
        if site.changes_nothing and _returns(site.spelled) not in ("void", "")
    }

    for site in sites:
        if (
            GET_PREFIXED.match(site.name)
            and not site.changes_nothing
            and site.name in asking
        ):
            continue

        if not LOWER_CAMEL.match(site.name):
            found.append(
                Violation(
                    site.path,
                    site.line,
                    site.column,
                    site.name,
                    NOT_LOWER_CAMEL,
                    "",
                    site.where,
                )
            )
            continue

        kind = _function_kind(site)

        if kind is not None:
            found.append(
                Violation(
                    site.path,
                    site.line,
                    site.column,
                    site.name,
                    kind,
                    "",
                    site.where,
                )
            )

    return found


def judge(
    sites: list[Site], declared: frozenset[str], namespaces: frozenset[str]
) -> list[Violation]:
    crowded = set()
    seen: dict[tuple[Path, int, str], int] = {}

    for site in sites:
        key = (site.path, site.scope, _bare(site.spelled))
        seen[key] = seen.get(key, 0) + 1

    for key, count in seen.items():
        if count > 1:
            crowded.add(key)

    found = []

    for site in sites:
        line, column = site.line, site.column

        if site.where == "constant":
            if not CONSTANT_SPELLED.match(site.name):
                found.append(
                    Violation(
                        site.path,
                        line,
                        column,
                        site.name,
                        CONSTANT_SPELLING,
                        "",
                        site.where,
                    )
                )
                continue
        elif not LOWER_CAMEL.match(site.name):
            found.append(
                Violation(
                    site.path,
                    line,
                    column,
                    site.name,
                    NOT_LOWER_CAMEL,
                    "",
                    site.where,
                )
            )
            continue

        core, plural = unwrap(site.spelled)

        if not is_project_type(core, declared, namespaces):
            if not names_a_thing(site.name, site.spelled):
                found.append(
                    Violation(
                        site.path,
                        line,
                        column,
                        site.name,
                        PART_OF_SPEECH,
                        "",
                        site.where,
                    )
                )

            continue

        if carries_type(site.name, site.spelled):
            continue

        kind = _kind_of(
            site,
            (site.path, site.scope, _bare(site.spelled)) in crowded,
            plural,
            is_math_type(core),
        )

        found.append(
            Violation(
                site.path, line, column, site.name, kind, core, site.where
            )
        )

    return found


def read_sources(root: Path) -> dict[Path, str]:
    sources = {}

    for pattern in CPP_GLOBS:
        for path in find_paths(root, pattern):
            text = path.read_text(encoding="utf-8", errors="ignore")
            sources[path] = _blank_init_lists(
                _blank_directives(mask_cpp(text))
            )

    return sources


def judge_part_of_speech(sites: list[Site]) -> list[Violation]:
    found = []

    for site in sites:
        if not LOWER_CAMEL.match(site.name):
            continue

        if names_a_thing(site.name, site.spelled):
            continue

        found.append(
            Violation(
                site.path,
                site.line,
                site.column,
                site.name,
                PART_OF_SPEECH,
                "",
                site.where,
            )
        )

    return found


def find_violations(root: Path) -> list[Violation]:
    sources = read_sources(root)
    declared = declared_types(sources)
    namespaces = project_namespaces(root)
    sites: list[Site] = []
    aside: list[Site] = []

    for path, masked in sources.items():
        sites.extend(sites_in(path, masked))
        aside.extend(
            replace(site, line=where[0], column=where[1])
            for site, where in (
                (one, _where(masked, one.offset))
                for one in stepped_over(path, masked)
            )
        )

    functions = [one for one in sites if one.where == "function"]
    values = [one for one in sites if one.where != "function"]

    return (
        judge(values, declared, namespaces)
        + judge_functions(functions)
        + judge_part_of_speech(aside)
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--root",
        type=Path,
        default=DEFAULT_ROOT,
        help="Repository root (defaults to the parent of scripts/)",
    )
    args = parser.parse_args()

    violations = find_violations(args.root)

    if not violations:
        print("OK: every name is a noun, carrying its type and its case.")
        return 0

    print(f"Found {len(violations)} naming violation(s):")
    print()

    for kind in KIND_ORDER:
        count = sum(1 for one in violations if one.kind == kind)

        if count:
            print(f"  {kind}: {count}")

    print()

    for one in violations:
        print(f"{one.path}:{one.line}:{one.column}: {one.name}")
        print(f"    {one.wanted()}")

    print()
    print(
        "Name a value for the thing it is, not for a preposition, "
        "an adjective or a tense."
    )

    return 1


if __name__ == "__main__":
    sys.exit(main())
