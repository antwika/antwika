#!/usr/bin/env python3

import contextlib
import importlib.util
import io
import sys
import tempfile
from pathlib import Path

SCRIPT_PATH = (
    Path(__file__).resolve().parent.parent / "check_include_order.py"
)

sys.path.insert(0, str(SCRIPT_PATH.parent))

spec = importlib.util.spec_from_file_location(
    "check_include_order", SCRIPT_PATH
)
check_include_order = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check_include_order)

m = check_include_order


def write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)


def run_main(root: Path) -> tuple[int, str]:
    old_argv = sys.argv
    sys.argv = ["check", "--root", str(root)]
    stdout = io.StringIO()

    try:
        with contextlib.redirect_stdout(stdout):
            exit_code = m.main()
    finally:
        sys.argv = old_argv

    return exit_code, stdout.getvalue()


def violations(source: str, stem: str) -> list[tuple[int, str]]:
    return m.find_include_violations(source, stem)


def it_accepts_the_canonical_include_order() -> None:
    source = (
        '#include "antwika/game/GridScene.hpp"\n'
        "\n"
        "#include <gtest/gtest.h>\n"
        "\n"
        "#include <algorithm>\n"
        "\n"
        "#include <antwika/gfx/Color.hpp>\n"
        "\n"
        '#include "antwika/game/Direction.hpp"\n'
    )

    assert violations(source, "GridScene") == []


def it_classifies_each_include_group() -> None:
    cases = {
        '"antwika/game/GridScene.hpp"': "own",
        '"antwika/game/Direction.hpp"': "project-quoted",
        "<cstdint>": "std",
        "<antwika/gfx/Color.hpp>": "project-angled",
        "<gtest/gtest.h>": "third-party",
    }

    for include, group in cases.items():
        assert m.include_group(include, "GridScene") == group


def it_reads_the_own_header_from_a_bare_quoted_include() -> None:
    assert m.include_group('"GridScene.hpp"', "GridScene") == "own"


def it_flags_a_standard_header_before_the_own_header() -> None:
    source = (
        "#include <algorithm>\n"
        "\n"
        '#include "antwika/game/GridScene.hpp"\n'
    )

    assert violations(source, "GridScene") == [(3, m.OUT_OF_ORDER)]


def it_flags_a_third_party_header_after_the_standard_block() -> None:
    source = "#include <algorithm>\n\n#include <gtest/gtest.h>\n"

    assert violations(source, "Whatever") == [(3, m.OUT_OF_ORDER)]


def it_flags_every_include_out_of_order() -> None:
    source = (
        "#include <antwika/gfx/Color.hpp>\n"
        "\n"
        "#include <algorithm>\n"
        "\n"
        "#include <gtest/gtest.h>\n"
    )

    assert violations(source, "Whatever") == [
        (3, m.OUT_OF_ORDER),
        (5, m.OUT_OF_ORDER),
    ]


def it_flags_a_group_change_without_a_blank_line() -> None:
    source = (
        '#include "antwika/game/GridScene.hpp"\n'
        "#include <algorithm>\n"
    )

    assert violations(source, "GridScene") == [(2, m.MISSING_SEPARATOR)]


def it_allows_a_blank_line_inside_one_group() -> None:
    source = (
        "#include <gtest/gtest.h>\n"
        "\n"
        "#include <glm/vec3.hpp>\n"
        "\n"
        "#include <algorithm>\n"
    )

    assert violations(source, "Whatever") == []


def it_allows_adjacent_includes_of_one_group() -> None:
    source = "#include <algorithm>\n#include <vector>\n"

    assert violations(source, "Whatever") == []


def it_allows_a_group_to_be_absent() -> None:
    source = (
        '#include "antwika/game/GridScene.hpp"\n'
        "\n"
        "#include <cstdint>\n"
    )

    assert violations(source, "GridScene") == []


def it_ignores_includes_inside_a_preprocessor_conditional() -> None:
    source = (
        '#include "SocketApi.hpp"\n'
        "\n"
        "#include <cstring>\n"
        "\n"
        "#ifdef _WIN32\n"
        "#include <ws2tcpip.h>\n"
        "#else\n"
        "#include <cerrno>\n"
        "#include <netinet/in.h>\n"
        "#endif\n"
    )

    assert violations(source, "SocketApi") == []


def it_stops_at_the_end_of_the_leading_include_block() -> None:
    source = (
        '#include "StbTrueType.hpp"\n'
        "\n"
        "#include <cstddef>\n"
        "\n"
        '#include "antwika/font/FontError.hpp"\n'
        "\n"
        "#define STB_TRUETYPE_IMPLEMENTATION\n"
        "\n"
        "#include <stb_truetype.h>\n"
    )

    assert violations(source, "StbTrueType") == []


def it_allows_pragma_once_above_the_include_block() -> None:
    source = "#pragma once\n\n#include <cstdint>\n"

    assert violations(source, "Thing") == []


def it_wants_a_blank_line_after_a_pragma_less_own_header() -> None:
    source = (
        "#pragma once\n"
        "#include <cstdint>\n"
        "#include <antwika/gfx/Color.hpp>\n"
    )

    assert violations(source, "Thing") == [(3, m.MISSING_SEPARATOR)]


def it_fails_on_an_include_out_of_order() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/src/A.cpp",
            "#include <algorithm>\n\n#include <gtest/gtest.h>\n",
        )

        exit_code, stdout = run_main(root)

        assert exit_code == 1
        assert m.OUT_OF_ORDER in stdout


def it_fails_on_a_missing_group_separator() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/src/A.cpp",
            '#include "antwika/a/A.hpp"\n#include <vector>\n',
        )

        exit_code, stdout = run_main(root)

        assert exit_code == 1
        assert m.MISSING_SEPARATOR in stdout


def it_names_the_file_and_line_of_a_violation() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/src/A.cpp",
            "#include <algorithm>\n\n#include <gtest/gtest.h>\n",
        )

        _, stdout = run_main(root)

        assert "src/libs/a/src/A.cpp:3" in stdout


def it_passes_on_a_conforming_tree() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/src/A.cpp",
            '#include "antwika/a/A.hpp"\n'
            "\n"
            "#include <vector>\n"
            "\n"
            "#include <antwika/log/ILogger.hpp>\n",
        )
        write(
            root / "backends/b/src/B.hpp",
            "#pragma once\n\n#include <cstdint>\n",
        )

        exit_code, stdout = run_main(root)

        assert exit_code == 0
        assert "OK:" in stdout


def it_leaves_every_real_include_block_in_order() -> None:
    found = m.find_violations(m.DEFAULT_ROOT)

    details = [f"{path}:{line}: {rule}" for path, line, rule in found]
    assert details == [], "\n".join(details)


def main() -> None:
    tests = [
        it_accepts_the_canonical_include_order,
        it_classifies_each_include_group,
        it_reads_the_own_header_from_a_bare_quoted_include,
        it_flags_a_standard_header_before_the_own_header,
        it_flags_a_third_party_header_after_the_standard_block,
        it_flags_every_include_out_of_order,
        it_flags_a_group_change_without_a_blank_line,
        it_allows_a_blank_line_inside_one_group,
        it_allows_adjacent_includes_of_one_group,
        it_allows_a_group_to_be_absent,
        it_ignores_includes_inside_a_preprocessor_conditional,
        it_stops_at_the_end_of_the_leading_include_block,
        it_allows_pragma_once_above_the_include_block,
        it_wants_a_blank_line_after_a_pragma_less_own_header,
        it_fails_on_an_include_out_of_order,
        it_fails_on_a_missing_group_separator,
        it_names_the_file_and_line_of_a_violation,
        it_passes_on_a_conforming_tree,
        it_leaves_every_real_include_block_in_order,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
