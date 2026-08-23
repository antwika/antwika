#!/usr/bin/env python3

import importlib.util
import sys
import tempfile
from pathlib import Path

SCRIPT_PATH = (
    Path(__file__).resolve().parent.parent / "check_comment_style.py"
)

sys.path.insert(0, str(SCRIPT_PATH.parent))

spec = importlib.util.spec_from_file_location(
    "check_comment_style", SCRIPT_PATH
)
check_comment_style = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check_comment_style)

m = check_comment_style


def write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)


def run_main(root: Path, *extra: str) -> int:
    import contextlib
    import io as stdio
    import sys

    sys.argv = ["check", "--root", str(root), *extra]

    with contextlib.redirect_stdout(stdio.StringIO()):
        return m.main()


def find_include_order(source: str, stem: str) -> list[int]:
    return m.find_include_order_violations(source, stem)


def rules(source: str) -> list[str]:
    broken = []

    for comment in m.find_comments(source):
        broken.extend(m.rules_broken(comment))

    return broken


def it_accepts_a_file_with_no_comments() -> None:
    assert rules("int f()\n{\n    return 1;\n}\n") == []


def it_accepts_a_permitted_marker_on_its_own_line() -> None:
    assert rules("// GCOVR_EXCL_START\nint x = 1;\n") == []


def it_accepts_a_permitted_marker_trailing_code_in_a_body() -> None:
    source = "int f()\n{\n    return Node{ // GCOVR_EXCL_LINE\n    };\n}\n"

    assert rules(source) == []


def it_rejects_a_prose_comment_that_merely_mentions_a_marker() -> None:
    source = "// That is all the GCOVR_EXCL_LINE markers below cover.\n"

    assert rules(source) == [
        "'//' comment that is not a permitted tool marker"
    ]


def it_rejects_an_ordinary_line_comment() -> None:
    assert rules("// Wrap the column.\nint x = 1;\n") == [
        "'//' comment that is not a permitted tool marker"
    ]


def it_rejects_a_namespace_closing_banner() -> None:
    source = "namespace a\n{\n}\n// namespace a\n"

    assert rules(source) == [
        "'//' comment that is not a permitted tool marker"
    ]


def it_accepts_a_doxygen_block_with_a_brief() -> None:
    source = "/**\n * @brief Does a thing.\n */\nvoid f();\n"

    assert rules(source) == []


def it_rejects_a_doxygen_block_without_a_brief() -> None:
    source = "/**\n * Does a thing.\n */\nvoid f();\n"

    assert rules(source) == ["Doxygen block without '@brief'"]


def it_rejects_a_plain_block_comment() -> None:
    source = "/* Does a thing. */\nvoid f();\n"

    assert rules(source) == [
        "block comment that is not a Doxygen '/**' block"
    ]


def it_rejects_a_doxygen_block_inside_a_function_body() -> None:
    source = "void f()\n{\n    /** @brief No. */\n    int x = 1;\n}\n"

    assert "comment inside a function body" in rules(source)


def it_rejects_an_unfinished_work_marker() -> None:
    source = "/**\n * @brief TODO finish this.\n */\nvoid f();\n"

    assert "unfinished-work marker 'TODO'" in rules(source)


def it_rejects_every_unfinished_marker_spelling() -> None:
    for marker in m.UNFINISHED_MARKERS:
        source = f"/**\n * @brief {marker} later.\n */\nvoid f();\n"

        assert f"unfinished-work marker '{marker}'" in rules(source)


def it_ignores_a_double_slash_inside_a_string_literal() -> None:
    source = 'const char* u = "http://example.com";\n'

    assert rules(source) == []


def it_ignores_a_comment_opener_inside_a_raw_string() -> None:
    source = 'const char* s = R"(a // b and /* c */)";\n'

    assert rules(source) == []


def it_ignores_a_double_slash_inside_a_character_literal() -> None:
    source = "char c = '/';\nchar d = '\\\\';\n"

    assert rules(source) == []


def it_does_not_treat_a_class_body_as_a_function_body() -> None:
    source = "class A\n{\n    /** @brief A field. */\n    int x;\n};\n"

    assert rules(source) == []


def it_does_not_treat_a_braced_initialiser_as_a_function_body() -> None:
    source = "int a[] = {\n    1,\n};\n/** @brief A thing. */\nvoid f();\n"

    assert rules(source) == []


def it_treats_a_test_body_as_a_function_body() -> None:
    source = "TEST(A, B)\n{\n    // Wrap the column.\n    int x = 1;\n}\n"

    broken = rules(source)

    assert "comment inside a function body" in broken


def it_accepts_the_three_phase_markers_in_a_test() -> None:
    source = (
        "TEST(GridSceneTest, Step_LeavesAFrameBehind)\n"
        "{\n"
        "    // Arrange\n"
        "    GridScene scene;\n"
        "\n"
        "    // Act\n"
        "    scene.step(1.0F);\n"
        "\n"
        "    // Assert\n"
        "    EXPECT_NE(scene.frame(), nullptr);\n"
        "}\n"
    )

    assert rules(source) == []


def it_accepts_a_phase_marker_across_the_gtest_macros() -> None:
    for macro in ("TEST", "TEST_F", "TEST_P"):
        source = f"{macro}(ATest, A_DoesX)\n{{\n    // Act\n    f();\n}}\n"

        assert rules(source) == []


def it_rejects_a_phase_marker_outside_a_test_body() -> None:
    source = "void f()\n{\n    // Arrange\n    int x = 1;\n}\n"

    assert "comment inside a function body" in rules(source)


def it_rejects_a_phase_marker_trailing_a_statement() -> None:
    source = "TEST(ATest, A_DoesX)\n{\n    int x = 1; // Arrange\n}\n"

    assert "comment inside a function body" in rules(source)


def it_rejects_a_phase_marker_spelt_differently() -> None:
    for body in ("Arrange:", "arrange", "Setup", "ARRANGE", "Act it out"):
        source = f"TEST(ATest, A_DoesX)\n{{\n    // {body}\n    f();\n}}\n"

        assert "comment inside a function body" in rules(source), body


def it_rejects_an_ordinary_comment_beside_a_phase_marker() -> None:
    source = (
        "TEST(ATest, A_DoesX)\n"
        "{\n"
        "    // Arrange\n"
        "    // Wrap the column so the map behaves as a torus.\n"
        "    int x = 1;\n"
        "}\n"
    )

    assert rules(source) == [
        "comment inside a function body",
        "'//' comment that is not a permitted tool marker",
    ]


def it_rejects_a_phase_marker_written_as_a_block() -> None:
    source = "TEST(ATest, A_DoesX)\n{\n    /** @brief Act */\n    f();\n}\n"

    assert "comment inside a function body" in rules(source)


def it_rejects_a_phase_marker_above_a_test_body() -> None:
    source = "// Arrange\nTEST(ATest, A_DoesX)\n{\n    f();\n}\n"

    assert rules(source) == [
        "'//' comment that is not a permitted tool marker"
    ]


def it_keeps_a_phase_marker_out_of_a_helper_beside_a_test() -> None:
    source = (
        "TEST(ATest, A_DoesX)\n"
        "{\n"
        "    // Act\n"
        "    f();\n"
        "}\n"
        "\n"
        "void helper()\n"
        "{\n"
        "    // Act\n"
        "    g();\n"
        "}\n"
    )

    assert "comment inside a function body" in rules(source)


def it_reports_the_line_the_comment_starts_on() -> None:
    source = "int x = 1;\nint y = 2;\n// A comment.\n"

    comments = m.find_comments(source)

    assert len(comments) == 1
    assert comments[0].line == 3


def it_flags_a_python_comment() -> None:
    assert m.find_python_comments("x = 1  # no\n") == [1]


def it_allows_a_python_shebang() -> None:
    assert m.find_python_comments("#!/usr/bin/env python3\nx = 1\n") == []


def it_ignores_a_hash_inside_a_python_string() -> None:
    assert m.find_python_comments('u = "http://a#b"\n') == []


def it_flags_a_cmake_comment() -> None:
    assert m.find_hash_comments("# A comment.\nset(x 1)\n", True) == [1]


def it_ignores_a_hash_inside_a_multiline_cmake_string() -> None:
    source = 'set(content\n"// Generated.\n#include <cstddef>\n")\n'

    assert m.find_hash_comments(source, True) == []


def it_flags_a_yaml_comment() -> None:
    assert m.find_hash_comments("# A comment.\nname: Build\n", False) == [1]


def it_flags_a_shell_comment_inside_a_yaml_block_scalar() -> None:
    source = "run: |\n  # A shell comment.\n  echo hi\n"

    assert m.find_hash_comments(source, False) == [2]


def it_ignores_a_shell_parameter_expansion_containing_a_hash() -> None:
    source = 'echo "${GITHUB_REF#refs/tags/v}"\n'

    assert m.find_hash_comments(source, False) == []


def it_allows_a_shell_shebang() -> None:
    source = "#!/usr/bin/env bash\nset -e\n"

    assert m.find_hash_comments(source, False) == []


def it_does_not_let_an_apostrophe_mask_a_later_yaml_comment() -> None:
    source = "name: it's fine\n# A comment.\n"

    assert m.find_hash_comments(source, False) == [2]


def it_allows_a_test_name_at_exactly_the_limit() -> None:
    name = "A" * m.MAX_TEST_NAME_LENGTH

    assert m.find_long_test_names(f"TEST(Fixture, {name})\n") == []


def it_rejects_a_test_name_one_character_over() -> None:
    name = "A" * (m.MAX_TEST_NAME_LENGTH + 1)

    found = m.find_long_test_names(f"TEST(Fixture, {name})\n")

    assert found == [(1, name)]


def it_checks_test_names_across_the_gtest_macros() -> None:
    name = "A" * (m.MAX_TEST_NAME_LENGTH + 1)

    for macro in ("TEST", "TEST_F", "TEST_P"):
        source = f"{macro}(Fixture, {name})\n"

        assert m.find_long_test_names(source) == [(1, name)]


def it_reports_the_line_a_wrapped_test_name_sits_on() -> None:
    name = "B" * (m.MAX_TEST_NAME_LENGTH + 1)

    found = m.find_long_test_names(f"TEST(Fixture,\n     {name})\n")

    assert found == [(2, name)]


def it_leaves_the_longest_real_test_name_alone() -> None:
    name = "DistributePots_GivesAnOddChipToTheFirstSeatLeftOfTheButton"

    assert m.find_long_test_names(f"TEST(HoldemPotsTest, {name})\n") == []


def it_flags_a_test_name_without_an_underscore() -> None:
    source = "TEST(WaveTest, AWavesSizeIsTheSumOfItsEntries)\n"

    assert m.find_ungrammatical_test_names(source) == [
        (1, "AWavesSizeIsTheSumOfItsEntries")
    ]


def it_accepts_a_method_does_x_name() -> None:
    source = "TEST(WaveTest, Size_IsTheSumOfItsEntries)\n"

    assert m.find_ungrammatical_test_names(source) == []


def it_fails_on_a_test_name_that_is_not_method_does_x() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "src/libs/a/tests/A.cpp", "TEST(ATest, NoUnderscore)\n")

        violations = m.find_violations(root)

        assert [v.rule for v in violations] == [m.TEST_NAME_GRAMMAR]
        assert not violations[0].migrating()
        assert run_main(root) == 1


def it_has_no_rule_left_under_migration() -> None:
    assert m.MIGRATING_RULES == frozenset()


def stray(root: Path) -> list[str]:
    return [
        p.relative_to(root).as_posix() for p in m.find_stray_markdown(root)
    ]


def it_allows_the_readme() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "README.md", "# antwika\n")

        assert stray(root) == []


def it_counts_a_docs_page_as_stray_markdown() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "docs/STYLE_GUIDE.md", "# Style\n")

        assert stray(root) == ["docs/STYLE_GUIDE.md"]


def it_fails_on_a_docs_directory() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "docs/STYLE_GUIDE.md", "# Style\n")

        assert [
            p.relative_to(root).as_posix()
            for p in m.find_docs_directory(root)
        ] == ["docs"]


def it_passes_a_tree_with_no_docs_directory() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "README.md", "# antwika\n")

        assert m.find_docs_directory(root) == []


def it_allows_the_generated_changelog() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "CHANGELOG.md", "# 1.0.0\n")

        assert stray(root) == []


def it_flags_a_changelog_that_is_not_the_root_one() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "src/libs/ui/CHANGELOG.md", "# 1.0.0\n")

        assert stray(root) == ["src/libs/ui/CHANGELOG.md"]


def it_flags_a_markdown_file_beside_an_asset() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "README.md", "# antwika\n")
        write(root / "assets/fonts/README.md", "# Bundled fonts\n")

        assert stray(root) == ["assets/fonts/README.md"]


def it_flags_a_readme_that_is_not_the_root_one() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "src/libs/ui/README.md", "# ui\n")

        assert stray(root) == ["src/libs/ui/README.md"]


def it_ignores_markdown_under_the_agent_directory() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / ".claude/skills/a/SKILL.md", "# A skill\n")

        assert stray(root) == []


def it_ignores_markdown_under_any_build_directory() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "build/notes.md", "# Generated\n")
        write(root / "build-coverage/notes.md", "# Generated\n")

        assert stray(root) == []


def it_does_not_treat_a_docs_lookalike_as_the_docs_directory() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "docsite/Guide.md", "# Guide\n")

        assert stray(root) == ["docsite/Guide.md"]
        assert m.find_docs_directory(root) == []


def it_fails_on_a_stray_markdown_file() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "assets/fonts/README.md", "# Bundled fonts\n")

        violations = m.find_violations(root)

        assert [v.rule for v in violations] == [m.STRAY_MARKDOWN]
        assert not violations[0].migrating()
        assert run_main(root) == 1


def it_keeps_the_repository_to_its_three_documents() -> None:
    root = Path(__file__).resolve().parent.parent.parent

    assert m.find_stray_markdown(root) == []


def streams(source: str) -> list[int]:
    return m.find_prose_streams(source)


def it_flags_prose_streamed_into_an_assertion() -> None:
    source = 'EXPECT_EQ(scene.level(), below) << "grew a tick early";\n'

    assert streams(source) == [1]


def it_flags_a_label_in_front_of_a_value() -> None:
    source = 'EXPECT_NE(name, "unknown") << "at index " << index;\n'

    assert streams(source) == [1]


def it_allows_a_streamed_value() -> None:
    source = "EXPECT_EQ(scene.level(), below) << index;\n"

    assert streams(source) == []


def it_allows_a_string_operand_with_no_stream() -> None:
    source = 'EXPECT_FALSE(actions.isActive("everything", state));\n'

    assert streams(source) == []


def it_allows_prose_streamed_into_fail() -> None:
    source = 'FAIL() << "expected an InputError";\n'

    assert streams(source) == []


def it_allows_prose_streamed_into_add_failure() -> None:
    source = 'ADD_FAILURE() << "no task with id " << taskId;\n'

    assert streams(source) == []


def it_finds_a_stream_that_starts_on_a_later_line() -> None:
    source = (
        "ASSERT_LT(polls, kPollLimit)\n"
        '    << "pollEvent never reported an empty queue";\n'
    )

    assert streams(source) == [1]


def it_finds_a_stream_after_a_multiline_argument_list() -> None:
    source = (
        "EXPECT_EQ(\n"
        "    screenToCell(cursor, zoomed),\n"
        "    before)\n"
        '    << "the cursor moved";\n'
    )

    assert streams(source) == [1]


def it_ignores_a_stream_belonging_to_the_next_statement() -> None:
    source = (
        "EXPECT_EQ(a, b);\n"
        'log << "not part of the assertion";\n'
    )

    assert streams(source) == []


def it_ignores_a_quote_inside_a_comment_after_the_assertion() -> None:
    source = 'EXPECT_EQ(a, b) << index; /** @brief "x" */\n'

    assert streams(source) == []


def it_ignores_a_shift_inside_the_streamed_expression() -> None:
    source = "EXPECT_EQ(a, b) << (mask << 2);\n"

    assert streams(source) == []


def it_reports_the_line_the_assertion_starts_on() -> None:
    source = (
        "\n"
        "\n"
        'EXPECT_TRUE(sawResize) << "never reported";\n'
    )

    assert streams(source) == [3]


def it_fails_on_streamed_prose() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/tests/ATest.cpp",
            'TEST(ATest, A_DoesX)\n{\n    EXPECT_TRUE(a) << "why";\n}\n',
        )

        violations = m.find_violations(root)

        assert [v.rule for v in violations] == [m.PROSE_IN_AN_ASSERTION]
        assert not violations[0].migrating()
        assert run_main(root) == 1


def fixtures(source: str) -> list[tuple[int, str]]:
    return m.find_ungrammatical_fixture_names(source)


def it_accepts_a_fixture_ending_in_test() -> None:
    source = "TEST(GridSceneTest, Sweep_RespectsClearOrdering)\n{\n}\n"

    assert fixtures(source) == []


def it_flags_a_fixture_with_another_suffix() -> None:
    source = "TEST(GridSceneSuite, Sweep_RespectsClearOrdering)\n{\n}\n"

    assert fixtures(source) == [(1, "GridSceneSuite")]


def it_flags_a_fixture_that_leads_with_test() -> None:
    source = "TEST(TestGridScene, Sweep_RespectsClearOrdering)\n{\n}\n"

    assert fixtures(source) == [(1, "TestGridScene")]


def it_checks_fixtures_across_the_gtest_macros() -> None:
    source = (
        "TEST(OneSuite, A_DoesX)\n"
        "TEST_F(TwoSuite, B_DoesX)\n"
        "TEST_P(ThreeSuite, C_DoesX)\n"
    )

    assert [name for _, name in fixtures(source)] == [
        "OneSuite",
        "TwoSuite",
        "ThreeSuite",
    ]


def it_reports_the_line_a_wrapped_fixture_sits_on() -> None:
    source = (
        "\n"
        "TEST(\n"
        "    GridSceneSuite,\n"
        "    Sweep_RespectsClearOrdering)\n"
    )

    assert fixtures(source) == [(3, "GridSceneSuite")]


def it_ignores_a_test_macro_inside_a_string_literal() -> None:
    source = 'const char *s = "TEST(GridSceneSuite, A_DoesX)";\n'

    assert fixtures(source) == []


def it_leaves_every_real_fixture_name_alone() -> None:
    root = Path(__file__).resolve().parent.parent.parent

    for pattern in m.CPP_GLOBS:
        for path in root.glob(pattern):
            text = path.read_text(encoding="utf-8", errors="ignore")

            assert m.find_ungrammatical_fixture_names(text) == []


def it_fails_on_a_fixture_without_the_suffix() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/tests/ATest.cpp",
            "TEST(ASuite, A_DoesX)\n{\n}\n",
        )

        violations = m.find_violations(root)

        assert [v.rule for v in violations] == [m.FIXTURE_SUFFIX]
        assert not violations[0].migrating()
        assert run_main(root) == 1


def it_flags_a_doxygen_block_on_a_struct() -> None:
    source = (
        "/**\n"
        " * @brief A node in the layout tree.\n"
        " */\n"
        "struct Node\n"
        "{\n"
        "    Rect bounds;\n"
        "};\n"
    )

    assert m.find_blocks_on_types(source) == [1]


def it_flags_a_block_separated_from_its_type_by_a_blank_line() -> None:
    source = "/** @brief A node. */\n\nclass Node\n{\n};\n"

    assert m.find_blocks_on_types(source) == [1]


def it_looks_past_a_template_header_and_an_attribute() -> None:
    source = (
        "/** @brief A node. */\n"
        "template <typename T>\n"
        "[[nodiscard]]\n"
        "class Node\n"
        "{\n"
        "};\n"
    )

    assert m.find_blocks_on_types(source) == [1]


def it_leaves_a_block_on_a_function_alone() -> None:
    source = (
        "/**\n"
        " * @brief Advances the scene by one tick.\n"
        " */\n"
        "std::uint64_t step(float dt);\n"
    )

    assert m.find_blocks_on_types(source) == []


def it_leaves_a_block_on_a_data_member_alone() -> None:
    source = (
        "struct Node\n"
        "{\n"
        "    /**\n"
        "     * @brief The layout box.\n"
        "     */\n"
        "    Rect bounds;\n"
        "};\n"
    )

    assert m.find_blocks_on_types(source) == []


def it_leaves_a_block_above_a_variable_of_a_class_type_alone() -> None:
    source = "/** @brief A spare node. */\nNode classroom;\n"

    assert m.find_blocks_on_types(source) == []


def it_ignores_a_type_declared_inside_a_string_literal() -> None:
    source = '/** @brief A hint. */\nconst char *s = "class Node {};";\n'

    assert m.find_blocks_on_types(source) == []


def it_fails_on_a_block_attached_to_a_type() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/include/Node.hpp",
            "/** @brief A node. */\nstruct Node final\n{\n};\n",
        )

        violations = m.find_violations(root)

        assert [v.rule for v in violations] == [m.BLOCK_ON_A_TYPE]
        assert not violations[0].migrating()
        assert run_main(root) == 1


def doubles(source: str) -> list[tuple[int, str]]:
    return m.find_test_double_violations(source)


def it_accepts_a_mock_declared_with_gmock() -> None:
    source = (
        "class MockLogger : public ILogger\n"
        "{\n"
        "public:\n"
        "    MOCK_METHOD(void, log, (Level level), (override));\n"
        "};\n"
    )

    assert doubles(source) == []


def it_accepts_a_hand_written_fake() -> None:
    source = (
        "class FakeRng : public IRng\n"
        "{\n"
        "public:\n"
        "    std::uint32_t next() noexcept override { return value; }\n"
        "\n"
        "    std::uint32_t value = 0;\n"
        "};\n"
    )

    assert doubles(source) == []


def it_flags_a_mock_without_gmock() -> None:
    source = (
        "class MockLogger : public ILogger\n"
        "{\n"
        "    void log(Level level) override {}\n"
        "};\n"
    )

    assert doubles(source) == [(1, m.MOCK_WITHOUT_GMOCK)]


def it_flags_a_fake_built_with_gmock() -> None:
    source = (
        "struct FakeRng : IRng\n"
        "{\n"
        "    MOCK_METHOD(std::uint32_t, next, (), (override));\n"
        "};\n"
    )

    assert doubles(source) == [(1, m.FAKE_WITH_GMOCK)]


def it_judges_each_double_by_its_own_body() -> None:
    source = (
        "class MockLogger\n"
        "{\n"
        "    MOCK_METHOD(void, log, (), (override));\n"
        "};\n"
        "\n"
        "class MockClock\n"
        "{\n"
        "    Time now() const { return {}; }\n"
        "};\n"
    )

    assert doubles(source) == [(6, m.MOCK_WITHOUT_GMOCK)]


def it_ignores_a_forward_declaration() -> None:
    source = "class MockLogger;\n\nclass FakeRng;\n"

    assert doubles(source) == []


def it_ignores_a_name_that_merely_starts_with_the_letters() -> None:
    source = (
        "class Mockery\n"
        "{\n"
        "    void jeer() {}\n"
        "};\n"
        "\n"
        "struct Faker\n"
        "{\n"
        "    MOCK_METHOD(void, go, (), ());\n"
        "};\n"
    )

    assert doubles(source) == []


def it_accepts_an_underscore_after_the_prefix() -> None:
    source = "class Mock_Logger\n{\n    void log() {}\n};\n"

    assert doubles(source) == [(1, m.MOCK_WITHOUT_GMOCK)]


def it_does_not_read_a_mock_method_out_of_a_neighbouring_double() -> None:
    source = (
        "class MockClock\n"
        "{\n"
        "    Time now() const { return {}; }\n"
        "};\n"
        "\n"
        "MOCK_METHOD(void, stray, (), ());\n"
    )

    assert doubles(source) == [(1, m.MOCK_WITHOUT_GMOCK)]


def it_ignores_a_mock_method_inside_a_string_literal() -> None:
    source = (
        "class MockLogger\n"
        "{\n"
        '    const char *hint = "MOCK_METHOD";\n'
        "};\n"
    )

    assert doubles(source) == [(1, m.MOCK_WITHOUT_GMOCK)]


def it_ignores_a_test_double_named_inside_a_string_literal() -> None:
    source = 'const char *name = "class MockLogger { };";\n'

    assert doubles(source) == []


def it_ignores_a_mock_method_inside_a_comment() -> None:
    source = (
        "class MockLogger\n"
        "{\n"
        "    /** @brief MOCK_METHOD once lived here. */\n"
        "    void log() {}\n"
        "};\n"
    )

    assert doubles(source) == [(1, m.MOCK_WITHOUT_GMOCK)]


def it_keeps_a_masked_source_the_same_length_and_shape() -> None:
    source = 'int a = 1;\n// MOCK_METHOD\nconst char *s = "x";\n'
    masked = m.mask_cpp(source)

    assert len(masked) == len(source)
    assert masked.count("\n") == source.count("\n")
    assert "MOCK_METHOD" not in masked
    assert masked.startswith("int a = 1;")


def it_fails_on_a_misnamed_test_double() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/tests/mocks/include/antwika/a/mocks"
            "/MockClock.hpp",
            "class MockClock final\n{\n    Time now() const;\n};\n",
        )

        violations = m.find_violations(root)

        assert [v.rule for v in violations] == [m.MOCK_WITHOUT_GMOCK]
        assert not violations[0].migrating()
        assert run_main(root) == 1


def it_accepts_the_canonical_include_order() -> None:
    source = (
        '#include "antwika/game/GridScene.hpp"\n'
        "#include <gtest/gtest.h>\n"
        "#include <algorithm>\n"
        "#include <antwika/gfx/Color.hpp>\n"
        '#include "antwika/game/Direction.hpp"\n'
    )

    assert find_include_order(source, "GridScene") == []


def it_flags_a_standard_header_before_the_own_header() -> None:
    source = (
        "#include <algorithm>\n"
        '#include "antwika/game/GridScene.hpp"\n'
    )

    assert find_include_order(source, "GridScene") == [2]


def it_flags_a_third_party_header_after_the_standard_block() -> None:
    source = "#include <algorithm>\n#include <gtest/gtest.h>\n"

    assert find_include_order(source, "Whatever") == [2]


def it_allows_a_group_to_be_absent() -> None:
    source = '#include "antwika/game/GridScene.hpp"\n#include <cstdint>\n'

    assert find_include_order(source, "GridScene") == []


def it_reports_only_the_first_include_out_of_order() -> None:
    source = (
        "#include <algorithm>\n"
        "#include <gtest/gtest.h>\n"
        "#include <cstdint>\n"
    )

    assert find_include_order(source, "Whatever") == [2]


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


def it_fails_on_an_include_out_of_order() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/src/A.cpp",
            "#include <algorithm>\n#include <gtest/gtest.h>\n",
        )

        violations = m.find_violations(root)

        assert [v.rule for v in violations] == [m.INCLUDE_ORDER]
        assert not violations[0].migrating()
        assert run_main(root) == 1


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

    assert find_include_order(source, "SocketApi") == []


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

    assert find_include_order(source, "StbTrueType") == []


def it_allows_pragma_once_above_the_include_block() -> None:
    source = "#pragma once\n\n#include <cstdint>\n"

    assert find_include_order(source, "Thing") == []


def it_gates_dockerfiles_but_keeps_parser_directives() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / ".devcontainer/base/Dockerfile",
            "# syntax=docker/dockerfile:1\n# A comment.\nFROM ubuntu\n",
        )

        violations = m.find_violations(root)

        assert len(violations) == 1
        assert violations[0].line == 2


def it_finds_violations_across_the_configured_globs() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "src/libs/a/src/A.cpp", "// nope\n")
        write(root / "backends/raylib/src/B.hpp", "// nope\n")
        write(root / "scripts/gated.py", "# nope\n")
        write(root / "cmake/Gated.cmake", "# nope\n")
        write(root / ".github/workflows/gated.yml", "# nope\n")

        violations = m.find_violations(root)
        paths = [v.path for v in violations]

        assert len(violations) == 5
        assert root / "src/libs/a/src/A.cpp" in paths
        assert root / "backends/raylib/src/B.hpp" in paths
        assert root / "scripts/gated.py" in paths
        assert root / "cmake/Gated.cmake" in paths
        assert root / ".github/workflows/gated.yml" in paths


def it_fails_by_default_and_only_warns_with_the_migration_flag() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "src/libs/a/src/A.cpp", "// nope\n")

        assert run_main(root) == 1
        assert run_main(root, "--warn-only") == 0


def unfinal(source: str) -> list[int]:
    masked = m.mask_cpp(source)

    return m.find_unfinal_types(masked, m.derivable_names(masked))


def it_accepts_a_leaf_type_marked_final() -> None:
    assert unfinal("struct Node final\n{\n    int x;\n};\n") == []


def it_flags_a_leaf_type_that_is_not_final() -> None:
    assert unfinal("struct Node\n{\n    int x;\n};\n") == [1]


def it_exempts_a_type_something_derives_from() -> None:
    source = (
        "class Base\n"
        "{\n"
        "};\n"
        "class Derived final : public Base\n"
        "{\n"
        "};\n"
    )

    assert unfinal(source) == []


def it_exempts_a_gtest_fixture() -> None:
    source = (
        "class ATest : public ::testing::Test\n"
        "{\n"
        "};\n"
        "TEST_F(ATest, A_DoesX)\n"
        "{\n"
        "}\n"
    )

    assert unfinal(source) == []


def it_exempts_a_double_wrapped_by_gmock() -> None:
    source = "class MockJob\n{\n};\n\nNiceMock<MockJob> job;\n"

    assert unfinal(source) == []


def it_exempts_a_typed_suite() -> None:
    source = (
        "template <typename T>\n"
        "class ASuiteTest\n"
        "{\n"
        "};\n"
        "TYPED_TEST_SUITE_P(ASuiteTest);\n"
    )

    assert unfinal(source) == []


def it_ignores_a_forward_declared_type() -> None:
    assert unfinal("struct Node;\n") == []


def it_does_not_read_a_type_head_out_of_a_string() -> None:
    assert unfinal('const char *s = "struct Node { int x; };";\n') == []


def it_does_not_read_an_enum_as_a_type_head() -> None:
    source = "enum class Kind : std::uint8_t\n{\n    A,\n};\n"

    assert unfinal(source) == []


def it_reaches_a_nested_type() -> None:
    source = (
        "struct Outer final\n"
        "{\n"
        "    struct Inner\n"
        "    {\n"
        "    };\n"
        "};\n"
    )

    assert unfinal(source) == [3]


def it_fails_on_a_type_that_is_not_final() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/include/antwika/a/Node.hpp",
            "struct Node\n{\n    int x;\n};\n",
        )

        violations = m.find_violations(root)

        assert m.MISSING_FINAL in [v.rule for v in violations]
        assert run_main(root) == 1


def it_fails_on_a_header_leading_with_test() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/tests/TestMessages.hpp",
            "struct SampleMessages final\n{\n};\n",
        )

        violations = m.find_violations(root)

        assert m.HEADER_LEADS_WITH_TEST in [v.rule for v in violations]
        assert run_main(root) == 1


def it_allows_a_header_merely_containing_test() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/tests/SampleMessages.hpp",
            "struct SampleMessages final\n{\n};\n",
        )

        rules = [v.rule for v in m.find_violations(root)]

        assert m.HEADER_LEADS_WITH_TEST not in rules


def it_leaves_every_real_type_final_where_it_can_be() -> None:
    root = m.DEFAULT_ROOT
    sources = {
        path: path.read_text(encoding="utf-8", errors="ignore")
        for pattern in m.CPP_GLOBS
        for path in root.glob(pattern)
    }
    derivable: set[str] = set()

    for text in sources.values():
        derivable |= m.derivable_names(m.mask_cpp(text))

    for path, text in sources.items():
        found = m.find_unfinal_types(m.mask_cpp(text), derivable)

        assert found == [], f"{path}: {found}"


def enums(source: str) -> list[tuple[int, str]]:
    return m.find_loose_enums(source)


def it_accepts_a_scoped_sized_enum() -> None:
    source = "enum class MobKind : std::uint8_t\n{\n    Grunt = 0,\n};\n"

    assert enums(source) == []


def it_flags_a_scoped_enum_with_no_underlying_type() -> None:
    source = "enum class SolveOutcome\n{\n    Solved,\n};\n"

    assert enums(source) == [(1, m.UNSIZED_ENUM)]


def it_flags_an_unscoped_enum() -> None:
    source = "enum Level : std::uint8_t\n{\n    Trace,\n};\n"

    assert enums(source) == [(1, m.UNSCOPED_ENUM)]


def it_reports_an_unscoped_enum_before_an_unsized_one() -> None:
    source = "enum Level\n{\n    Trace,\n};\n"

    assert enums(source) == [(1, m.UNSCOPED_ENUM)]


def it_accepts_an_enum_struct() -> None:
    source = "enum struct Side : std::uint8_t\n{\n    North,\n};\n"

    assert enums(source) == []


def it_ignores_a_bare_forward_declaration() -> None:
    assert enums("enum class Level;\n") == []


def it_reads_a_sized_forward_declaration() -> None:
    assert enums("enum class Level : std::uint8_t;\n") == []


def it_does_not_read_an_enum_out_of_a_string_literal() -> None:
    assert enums('const char *s = "enum class Level { A };";\n') == []


def it_fails_on_an_unsized_enum() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/include/antwika/a/Level.hpp",
            "enum class Level\n{\n    Trace,\n};\n",
        )

        violations = m.find_violations(root)

        assert m.UNSIZED_ENUM in [v.rule for v in violations]
        assert run_main(root) == 1


def it_leaves_every_real_enum_alone() -> None:
    root = m.DEFAULT_ROOT

    for pattern in m.CPP_GLOBS:
        for path in root.glob(pattern):
            text = path.read_text(encoding="utf-8", errors="ignore")

            assert m.find_loose_enums(text) == [], path


def it_flags_a_function_with_no_return_annotation() -> None:
    assert m.find_unannotated_functions("def f(x: int):\n    pass\n") == [1]


def it_flags_a_parameter_with_no_annotation() -> None:
    assert m.find_unannotated_functions("def f(x) -> None:\n    pass\n") == [1]


def it_accepts_a_fully_annotated_function() -> None:
    source = "def f(x: int) -> str:\n    return str(x)\n"

    assert m.find_unannotated_functions(source) == []


def it_accepts_a_bare_self() -> None:
    source = "class A:\n    def f(self) -> None:\n        pass\n"

    assert m.find_unannotated_functions(source) == []


def it_wants_an_annotation_on_star_args() -> None:
    source = "def f(*rest) -> None:\n    pass\n"

    assert m.find_unannotated_functions(source) == [1]


def it_ignores_a_python_file_it_cannot_parse_for_annotations() -> None:
    assert m.find_unannotated_functions("def f(:\n") == []


def it_fails_on_an_unannotated_function() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(root / "scripts/gated.py", "def f():\n    pass\n")

        violations = m.find_violations(root)

        assert m.MISSING_ANNOTATION in [v.rule for v in violations]
        assert run_main(root) == 1


def it_leaves_every_real_python_function_annotated() -> None:
    root = m.DEFAULT_ROOT

    for pattern in m.PYTHON_GLOBS:
        for path in root.glob(pattern):
            text = path.read_text(encoding="utf-8", errors="ignore")

            assert m.find_unannotated_functions(text) == [], path


def constants(source: str) -> list[int]:
    return m.find_unprefixed_constants(source)


def it_accepts_a_prefixed_constant() -> None:
    source = "namespace a\n{\n    constexpr int kMax = 3;\n}\n"

    assert constants(source) == []


def it_flags_a_namespace_scope_constant_without_the_prefix() -> None:
    source = "namespace a\n{\n    constexpr int max = 3;\n}\n"

    assert constants(source) == [3]


def it_reads_a_brace_initialised_constant() -> None:
    source = "namespace a\n{\n    constexpr Size slot{.width = 8};\n}\n"

    assert constants(source) == [3]


def it_leaves_a_constant_inside_a_body_alone() -> None:
    source = (
        "namespace a\n"
        "{\n"
        "    void f()\n"
        "    {\n"
        "        constexpr int max = 3;\n"
        "    }\n"
        "}\n"
    )

    assert constants(source) == []


def it_leaves_a_static_member_constant_alone() -> None:
    source = (
        "namespace a\n"
        "{\n"
        "    struct S\n"
        "    {\n"
        "        static constexpr int max = 3;\n"
        "    };\n"
        "}\n"
    )

    assert constants(source) == []


def it_does_not_read_a_constexpr_function_as_a_constant() -> None:
    source = (
        "namespace a\n"
        "{\n"
        "    constexpr int square(int x)\n"
        "    {\n"
        "        return x * x;\n"
        "    }\n"
        "}\n"
    )

    assert constants(source) == []


def it_does_not_read_a_constexpr_declaration_as_a_constant() -> None:
    source = "namespace a\n{\n    constexpr int square(int x);\n}\n"

    assert constants(source) == []


def it_reads_a_variable_template() -> None:
    source = (
        "namespace a\n"
        "{\n"
        "    template <typename T>\n"
        "    inline constexpr std::size_t count = 1;\n"
        "}\n"
    )

    assert constants(source) == [4]


def it_does_not_trip_on_a_templated_type() -> None:
    source = "namespace a\n{\n    constexpr std::array<int, kN> kAll{1};\n}\n"

    assert constants(source) == []


def it_does_not_read_a_constant_out_of_a_string_literal() -> None:
    source = (
        "namespace a\n"
        "{\n"
        '    const char *s = "constexpr int max = 3;";\n'
        "}\n"
    )

    assert constants(source) == []


def it_fails_on_a_constant_without_the_prefix() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/include/antwika/a/Limits.hpp",
            "namespace a\n{\n    constexpr int max = 3;\n}\n",
        )

        violations = m.find_violations(root)

        assert m.CONSTANT_WITHOUT_K in [v.rule for v in violations]
        assert run_main(root) == 1


def it_leaves_every_real_constant_alone() -> None:
    root = m.DEFAULT_ROOT

    for pattern in m.CPP_GLOBS:
        for path in root.glob(pattern):
            text = path.read_text(encoding="utf-8", errors="ignore")

            assert m.find_unprefixed_constants(text) == [], path


def abstract(source: str) -> list[int]:
    return m.find_abstract_types_without_i(source)


def it_accepts_an_abstract_type_with_the_prefix() -> None:
    source = "class ILogger\n{\n    virtual void log() = 0;\n};\n"

    assert abstract(source) == []


def it_flags_an_abstract_type_without_the_prefix() -> None:
    source = "class Logger\n{\n    virtual void log() = 0;\n};\n"

    assert abstract(source) == [1]


def it_leaves_a_type_with_a_virtual_body_alone() -> None:
    source = "class Logger\n{\n    virtual void log() {}\n};\n"

    assert abstract(source) == []


def it_does_not_read_a_default_member_initialiser_as_pure() -> None:
    source = "struct Node\n{\n    int depth = 0;\n};\n"

    assert abstract(source) == []


def it_flags_an_abstract_class_template() -> None:
    source = (
        "template <typename T>\n"
        "class Store\n"
        "{\n"
        "    virtual void keep(T value) = 0;\n"
        "};\n"
    )

    assert abstract(source) == [2]


def it_reads_a_pure_virtual_carrying_specifiers() -> None:
    source = (
        "class Sink\n"
        "{\n"
        "    virtual int count() const noexcept = 0;\n"
        "};\n"
    )

    assert abstract(source) == [1]


def it_does_not_read_an_abstract_type_out_of_a_string_literal() -> None:
    source = 'const char *s = "class Logger { virtual void f() = 0; };";\n'

    assert abstract(source) == []


def it_does_not_flag_a_name_merely_starting_with_i() -> None:
    source = "class Inspector\n{\n    virtual void look() = 0;\n};\n"

    assert abstract(source) == [1]


def it_fails_on_an_abstract_type_without_the_prefix() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/include/antwika/a/Sink.hpp",
            "class Sink\n{\n    virtual void take() = 0;\n};\n",
        )

        violations = m.find_violations(root)

        assert m.ABSTRACT_WITHOUT_I in [v.rule for v in violations]
        assert run_main(root) == 1


def it_leaves_every_real_abstract_type_alone() -> None:
    root = m.DEFAULT_ROOT

    for pattern in m.CPP_GLOBS:
        for path in root.glob(pattern):
            text = path.read_text(encoding="utf-8", errors="ignore")

            assert m.find_abstract_types_without_i(text) == [], path


def unsorted_sources(text: str) -> list[int]:
    return m.find_unsorted_source_lists(text)


def it_accepts_a_sorted_source_list() -> None:
    text = "    SOURCES\n        src/Action.cpp\n        src/AppMode.cpp\n"

    assert unsorted_sources(text) == []


def it_flags_a_source_list_out_of_order() -> None:
    text = "    SOURCES\n        src/AppMode.cpp\n        src/Action.cpp\n"

    assert unsorted_sources(text) == [2]


def it_reports_the_first_entry_out_of_place() -> None:
    text = (
        "    SOURCES\n"
        "        src/A.cpp\n"
        "        src/C.cpp\n"
        "        src/B.cpp\n"
        "        src/E.cpp\n"
    )

    assert unsorted_sources(text) == [3]


def it_leaves_a_single_entry_alone() -> None:
    text = (
        "set(ANTWIKA_GFX_FONT_SOURCE\n"
        "    generated/BuiltInFontBytes.cpp\n"
        ")\n"
    )

    assert unsorted_sources(text) == []


def it_checks_each_list_in_a_file_separately() -> None:
    text = (
        "add_library(one\n"
        "    src/A.cpp\n"
        "    src/B.cpp\n"
        ")\n"
        "add_library(two\n"
        "    src/Z.cpp\n"
        "    src/Y.cpp\n"
        ")\n"
    )

    assert unsorted_sources(text) == [6]


def it_does_not_join_two_lists_at_different_indents() -> None:
    text = "one\n    src/B.cpp\n)\ntwo\n        src/A.cpp\n)\n"

    assert unsorted_sources(text) == []


def it_does_not_read_a_keyword_as_a_source() -> None:
    text = "    src/B.cpp\n    PRIVATE\n    src/A.cpp\n"

    assert unsorted_sources(text) == []


def it_fails_on_an_unsorted_source_list() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/CMakeLists.txt",
            "add_library(a\n    src/B.cpp\n    src/A.cpp\n)\n",
        )

        violations = m.find_violations(root)

        assert [v.rule for v in violations] == [m.SOURCE_LIST_ORDER]
        assert run_main(root) == 1


def it_leaves_every_real_source_list_sorted() -> None:
    root = m.DEFAULT_ROOT

    for pattern in m.CMAKE_GLOBS:
        for path in root.glob(pattern):
            text = path.read_text(encoding="utf-8", errors="ignore")

            assert m.find_unsorted_source_lists(text) == [], path


def misnamed(source: str, stem: str) -> list[int]:
    return m.find_misnamed_headers(source, stem)


def it_accepts_a_header_named_for_its_one_type() -> None:
    source = (
        "#pragma once\n"
        "\n"
        "namespace antwika::game\n"
        "{\n"
        "    class SnapshotStore final\n"
        "    {\n"
        "    };\n"
        "}\n"
    )

    assert misnamed(source, "SnapshotStore") == []


def it_flags_a_header_not_named_for_its_one_type() -> None:
    source = (
        "#pragma once\n"
        "\n"
        "namespace antwika::game\n"
        "{\n"
        "    class GameSnapshotStore final\n"
        "    {\n"
        "    };\n"
        "}\n"
    )

    assert misnamed(source, "SnapshotStore") == [5]


def it_reports_the_line_the_type_is_declared_on() -> None:
    source = (
        "namespace a\n"
        "{\n"
        "    template <typename T>\n"
        "    class Ring\n"
        "    {\n"
        "    };\n"
        "}\n"
    )

    assert misnamed(source, "Buffer") == [4]


def it_leaves_a_header_of_free_functions_alone() -> None:
    source = (
        "namespace antwika::pattern\n"
        "{\n"
        "    struct Slice\n"
        "    {\n"
        "    };\n"
        "\n"
        "    Pattern pure(int value);\n"
        "}\n"
    )

    assert misnamed(source, "Patterns") == []


def it_leaves_a_header_of_several_types_alone() -> None:
    source = (
        "namespace a\n"
        "{\n"
        "    struct Pressed\n"
        "    {\n"
        "    };\n"
        "\n"
        "    struct Released\n"
        "    {\n"
        "    };\n"
        "}\n"
    )

    assert misnamed(source, "Events") == []


def it_leaves_a_header_holding_a_constant_alone() -> None:
    source = (
        "namespace a\n"
        "{\n"
        "    struct Rect\n"
        "    {\n"
        "    };\n"
        "\n"
        "    constexpr int kMax = 3;\n"
        "}\n"
    )

    assert misnamed(source, "Shapes") == []


def it_does_not_count_a_nested_type() -> None:
    source = (
        "namespace a\n"
        "{\n"
        "    class Outer\n"
        "    {\n"
        "        struct Inner\n"
        "        {\n"
        "        };\n"
        "    };\n"
        "}\n"
    )

    assert misnamed(source, "Outer") == []


def it_accepts_an_enum_with_an_underlying_type() -> None:
    source = (
        "namespace a\n"
        "{\n"
        "    enum class MobKind : std::uint8_t\n"
        "    {\n"
        "        Grunt = 0,\n"
        "    };\n"
        "}\n"
    )

    assert misnamed(source, "MobKind") == []


def it_does_not_read_a_type_out_of_a_string_literal() -> None:
    source = 'namespace a\n{\n    const char *s = "class Foo {";\n}\n'

    assert misnamed(source, "Bar") == []


def it_ignores_a_forward_declaration_above_the_type() -> None:
    source = (
        "namespace a\n"
        "{\n"
        "    class Painter;\n"
        "\n"
        "    class Canvas\n"
        "    {\n"
        "    };\n"
        "}\n"
    )

    assert misnamed(source, "Canvas") == []


def it_fails_on_a_header_not_named_for_its_type() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/include/antwika/a/Store.hpp",
            "namespace a\n{\n    class Keeper final\n    {\n    };\n}\n",
        )

        violations = m.find_violations(root)

        assert [v.rule for v in violations] == [m.HEADER_NAME]
        assert run_main(root) == 1


def it_leaves_every_real_header_name_alone() -> None:
    root = m.DEFAULT_ROOT

    for pattern in m.CPP_GLOBS:
        for path in root.glob(pattern):
            if path.suffix != ".hpp":
                continue

            text = path.read_text(encoding="utf-8", errors="ignore")

            assert m.find_misnamed_headers(text, path.stem) == [], path


def crowded(source: str) -> list[int]:
    return m.find_crowded_headers(source)


def it_accepts_a_header_holding_one_struct() -> None:
    source = (
        "#pragma once\n"
        "\n"
        "namespace antwika::game\n"
        "{\n"
        "    struct Tile final\n"
        "    {\n"
        "    };\n"
        "}\n"
    )

    assert crowded(source) == []


def it_flags_the_second_struct_in_a_header() -> None:
    source = (
        "#pragma once\n"
        "\n"
        "namespace antwika::game\n"
        "{\n"
        "    struct Tile final\n"
        "    {\n"
        "    };\n"
        "\n"
        "    struct Tilemap final\n"
        "    {\n"
        "    };\n"
        "}\n"
    )

    assert crowded(source) == [9]


def it_flags_every_type_after_the_first() -> None:
    source = (
        "namespace a\n"
        "{\n"
        "    struct One final\n"
        "    {\n"
        "    };\n"
        "    struct Two final\n"
        "    {\n"
        "    };\n"
        "    class Three final\n"
        "    {\n"
        "    };\n"
        "}\n"
    )

    assert crowded(source) == [6, 9]


def it_leaves_a_nested_type_out_of_the_count() -> None:
    source = (
        "namespace a\n"
        "{\n"
        "    class Scheduler final\n"
        "    {\n"
        "    private:\n"
        "        struct Entry final\n"
        "        {\n"
        "        };\n"
        "    };\n"
        "}\n"
    )

    assert crowded(source) == []


def it_leaves_a_forward_declaration_out_of_the_count() -> None:
    source = (
        "namespace a\n"
        "{\n"
        "    class Manager;\n"
        "\n"
        "    class World final\n"
        "    {\n"
        "    };\n"
        "}\n"
    )

    assert crowded(source) == []


def it_lets_an_enum_share_a_header_with_a_struct() -> None:
    source = (
        "namespace a\n"
        "{\n"
        "    enum class Kind : std::uint8_t\n"
        "    {\n"
        "        One,\n"
        "    };\n"
        "\n"
        "    struct Item final\n"
        "    {\n"
        "    };\n"
        "}\n"
    )

    assert crowded(source) == []


def it_flags_a_crowded_header_through_find_violations() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/include/antwika/a/Pair.hpp",
            "namespace a\n"
            "{\n"
            "    struct One final\n"
            "    {\n"
            "    };\n"
            "    struct Two final\n"
            "    {\n"
            "    };\n"
            "}\n",
        )

        rules = [v.rule for v in m.find_violations(root)]

        assert m.CROWDED_HEADER in rules


def it_leaves_every_real_header_holding_one_shape() -> None:
    root = m.DEFAULT_ROOT

    for pattern in m.CPP_GLOBS:
        for path in root.glob(pattern):
            if path.suffix != ".hpp":
                continue

            text = path.read_text(encoding="utf-8", errors="ignore")

            assert m.find_crowded_headers(text) == [], path


def unprefixed(source: str, relative: str) -> list[int]:
    return m.find_unprefixed_doubles(source, Path(relative))


def it_accepts_a_prefixed_double() -> None:
    source = "class FakeJob final : public IJob\n{\n};\n"

    assert unprefixed(source, "src/libs/a/tests/AJobTest.cpp") == []


def it_flags_a_double_without_a_prefix() -> None:
    source = "class RecordingJob final : public IJob\n{\n};\n"

    assert unprefixed(source, "src/libs/a/tests/AJobTest.cpp") == [1]


def it_leaves_a_type_outside_a_tests_directory_alone() -> None:
    source = "class RecordingJob final : public IJob\n{\n};\n"

    assert unprefixed(source, "src/libs/a/src/RecordingJob.cpp") == []


def it_only_reads_a_base_that_looks_like_an_interface() -> None:
    source = "class Recording final : public Job\n{\n};\n"

    assert unprefixed(source, "src/libs/a/tests/AJobTest.cpp") == []


def it_reads_a_qualified_interface_base() -> None:
    source = "class Recording final : public antwika::a::IJob\n{\n};\n"

    assert unprefixed(source, "src/libs/a/tests/AJobTest.cpp") == [1]


def it_does_not_read_a_double_out_of_a_string() -> None:
    source = 'const char *s = "class Recording : public IJob {};";\n'

    assert unprefixed(source, "src/libs/a/tests/AJobTest.cpp") == []


def it_fails_on_an_unprefixed_double() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/tests/AJobTest.cpp",
            "class RecordingJob final : public IJob\n{\n};\n",
        )

        violations = m.find_violations(root)
        mine = [v for v in violations if v.rule == m.DOUBLE_WITHOUT_A_PREFIX]

        assert len(mine) == 1
        assert not mine[0].migrating()
        assert run_main(root) == 1


def misplaced(source: str, relative: str) -> list[int]:
    return m.find_misplaced_doubles(source, Path(relative))


def it_accepts_a_double_in_the_published_tree() -> None:
    source = "class MockLogger : public ILogger\n{\n};\n"
    where = (
        "src/libs/log/tests/mocks/include/antwika/log/mocks/"
        "MockLogger.hpp"
    )

    assert misplaced(source, where) == []


def it_flags_a_library_double_beside_its_tests() -> None:
    source = "class FakeClock : public IClock\n{\n};\n"

    assert misplaced(source, "src/libs/time/tests/FakeClock.hpp") == [1]


def it_flags_a_library_double_declared_in_a_test_file() -> None:
    source = "TEST(AClockTest, Now_Advances)\n{\n}\n"
    source += "class FakeClock : public IClock\n{\n};\n"

    assert misplaced(source, "src/libs/time/tests/AClockTest.cpp") == [4]


def it_leaves_an_app_double_alone() -> None:
    source = "class FakeMenuCommands : public IMenuCommands\n{\n};\n"

    assert misplaced(source, "src/apps/game/tests/FakeMenuCommands.hpp") == []


def it_leaves_a_backend_double_alone() -> None:
    source = "class FakeSocket : public ISocket\n{\n};\n"

    assert misplaced(source, "backends/sockets/tests/FakeSocket.hpp") == []


def it_does_not_read_a_double_out_of_a_string_literal() -> None:
    source = 'const char *s = "class MockLogger : public ILogger";\n'

    assert misplaced(source, "src/libs/log/tests/ALogTest.cpp") == []


def it_wants_the_module_directory_to_match_the_include_path() -> None:
    source = "class MockLogger : public ILogger\n{\n};\n"
    where = (
        "src/libs/log/tests/mocks/include/antwika/log/MockLogger.hpp"
    )

    assert misplaced(source, where) == [1]


def it_fails_on_a_library_double_outside_the_tree() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "src/libs/a/tests/FakeThing.hpp",
            "class FakeThing : public IThing\n{\n};\n",
        )

        violations = m.find_violations(root)

        assert m.DOUBLE_OUTSIDE_THE_TREE in [v.rule for v in violations]
        assert run_main(root) == 1


def it_leaves_every_real_double_where_it_is() -> None:
    root = m.DEFAULT_ROOT

    for pattern in m.CPP_GLOBS:
        for path in root.glob(pattern):
            text = path.read_text(encoding="utf-8", errors="ignore")
            relative = path.relative_to(root)

            assert m.find_misplaced_doubles(text, relative) == [], relative


def it_accepts_a_typed_suite_ending_in_test() -> None:
    source = "TYPED_TEST_P(GfxBackendConformanceTest, Close_IsIdempotent)\n"

    assert fixtures(source) == []


def it_flags_a_typed_suite_without_the_suffix() -> None:
    source = "TYPED_TEST_P(GfxBackendConformance, Close_IsIdempotent)\n"

    assert fixtures(source) == [(1, "GfxBackendConformance")]


def it_reads_the_grammar_of_a_typed_test_name() -> None:
    source = "TYPED_TEST_P(AConformanceTest, ANewHostHoldsNoPeers)\n"

    assert m.find_ungrammatical_test_names(source) == [
        (1, "ANewHostHoldsNoPeers")
    ]


def it_reads_the_length_of_a_typed_test_name() -> None:
    name = "Load_" + "A" * m.MAX_TEST_NAME_LENGTH
    source = f"TYPED_TEST_P(AConformanceTest, {name})\n"

    assert m.find_long_test_names(source) == [(1, name)]


def it_exempts_a_phase_marker_in_a_typed_test_body() -> None:
    source = (
        "TYPED_TEST_P(AConformanceTest, Close_IsIdempotent)\n"
        "{\n"
        "    // Arrange\n"
        "    auto host = open();\n"
        "}\n"
    )

    assert rules(source) == []


def it_ignores_the_registration_and_instantiation_macros() -> None:
    source = (
        "TYPED_TEST_SUITE_P(GfxBackendConformanceTest);\n"
        "REGISTER_TYPED_TEST_SUITE_P(\n"
        "    GfxBackendConformanceTest,\n"
        "    Close_IsIdempotent);\n"
        "INSTANTIATE_TYPED_TEST_SUITE_P(\n"
        "    Raylib,\n"
        "    GfxBackendConformanceTest,\n"
        "    RaylibBackendTraits);\n"
    )

    assert fixtures(source) == []
    assert m.find_ungrammatical_test_names(source) == []


def it_flags_a_module_docstring() -> None:
    assert m.find_docstrings('"""What this script does."""\n\nx = 1\n') == [1]


def it_flags_a_function_docstring() -> None:
    source = 'def f():\n    """What f does."""\n    return 1\n'

    assert m.find_docstrings(source) == [2]


def it_flags_a_class_docstring() -> None:
    source = 'class E(Exception):\n    """Raised when."""\n\n    pass\n'

    assert m.find_docstrings(source) == [2]


def it_flags_a_docstring_written_with_one_quote() -> None:
    assert m.find_docstrings("def f():\n    'What f does.'\n    pass\n") == [2]


def it_reports_the_line_a_wrapped_docstring_opens_on() -> None:
    source = 'def f():\n    """First.\n\n    Second.\n    """\n    pass\n'

    assert m.find_docstrings(source) == [2]


def it_ignores_a_triple_quoted_string_bound_to_a_name() -> None:
    source = 'import re\n\nD = re.compile(r"""[)"\']+$""")\n'

    assert m.find_docstrings(source) == []


def it_ignores_a_string_that_is_not_the_first_statement() -> None:
    source = 'def f():\n    x = 1\n    "not a docstring"\n    return x\n'

    assert m.find_docstrings(source) == []


def it_ignores_a_returned_string() -> None:
    assert m.find_docstrings('def f():\n    return "text"\n') == []


def it_ignores_a_python_file_it_cannot_parse() -> None:
    assert m.find_docstrings("def f(:\n") == []


def it_fails_on_a_python_docstring() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        write(
            root / "scripts/gated.py",
            'def f() -> None:\n    """No."""\n',
        )

        violations = m.find_violations(root)

        assert [v.rule for v in violations] == [m.DOCSTRING]
        assert not violations[0].migrating()
        assert run_main(root) == 1


def it_leaves_every_script_free_of_docstrings() -> None:
    root = m.DEFAULT_ROOT

    for pattern in m.PYTHON_GLOBS:
        for path in root.glob(pattern):
            text = path.read_text(encoding="utf-8", errors="ignore")

            assert m.find_docstrings(text) == [], path


def it_keeps_every_source_comment_within_the_style_guide() -> None:
    violations = [
        v for v in m.find_violations(m.DEFAULT_ROOT) if not v.migrating()
    ]

    details = [f"{v.path}:{v.line}: {v.rule}" for v in violations]
    assert details == [], "\n".join(details)


def main() -> None:
    tests = [
        it_accepts_a_file_with_no_comments,
        it_accepts_a_permitted_marker_on_its_own_line,
        it_accepts_a_permitted_marker_trailing_code_in_a_body,
        it_rejects_a_prose_comment_that_merely_mentions_a_marker,
        it_rejects_an_ordinary_line_comment,
        it_rejects_a_namespace_closing_banner,
        it_accepts_a_doxygen_block_with_a_brief,
        it_rejects_a_doxygen_block_without_a_brief,
        it_rejects_a_plain_block_comment,
        it_rejects_a_doxygen_block_inside_a_function_body,
        it_rejects_an_unfinished_work_marker,
        it_rejects_every_unfinished_marker_spelling,
        it_ignores_a_double_slash_inside_a_string_literal,
        it_ignores_a_comment_opener_inside_a_raw_string,
        it_ignores_a_double_slash_inside_a_character_literal,
        it_does_not_treat_a_class_body_as_a_function_body,
        it_does_not_treat_a_braced_initialiser_as_a_function_body,
        it_treats_a_test_body_as_a_function_body,
        it_accepts_the_three_phase_markers_in_a_test,
        it_accepts_a_phase_marker_across_the_gtest_macros,
        it_rejects_a_phase_marker_outside_a_test_body,
        it_rejects_a_phase_marker_trailing_a_statement,
        it_rejects_a_phase_marker_spelt_differently,
        it_rejects_an_ordinary_comment_beside_a_phase_marker,
        it_rejects_a_phase_marker_written_as_a_block,
        it_rejects_a_phase_marker_above_a_test_body,
        it_keeps_a_phase_marker_out_of_a_helper_beside_a_test,
        it_reports_the_line_the_comment_starts_on,
        it_flags_a_python_comment,
        it_allows_a_python_shebang,
        it_ignores_a_hash_inside_a_python_string,
        it_flags_a_cmake_comment,
        it_ignores_a_hash_inside_a_multiline_cmake_string,
        it_flags_a_yaml_comment,
        it_flags_a_shell_comment_inside_a_yaml_block_scalar,
        it_ignores_a_shell_parameter_expansion_containing_a_hash,
        it_allows_a_shell_shebang,
        it_does_not_let_an_apostrophe_mask_a_later_yaml_comment,
        it_allows_a_test_name_at_exactly_the_limit,
        it_rejects_a_test_name_one_character_over,
        it_checks_test_names_across_the_gtest_macros,
        it_reports_the_line_a_wrapped_test_name_sits_on,
        it_leaves_the_longest_real_test_name_alone,
        it_flags_a_test_name_without_an_underscore,
        it_accepts_a_method_does_x_name,
        it_fails_on_a_test_name_that_is_not_method_does_x,
        it_has_no_rule_left_under_migration,
        it_allows_the_readme,
        it_counts_a_docs_page_as_stray_markdown,
        it_fails_on_a_docs_directory,
        it_passes_a_tree_with_no_docs_directory,
        it_allows_the_generated_changelog,
        it_flags_a_changelog_that_is_not_the_root_one,
        it_flags_a_markdown_file_beside_an_asset,
        it_flags_a_readme_that_is_not_the_root_one,
        it_ignores_markdown_under_the_agent_directory,
        it_ignores_markdown_under_any_build_directory,
        it_does_not_treat_a_docs_lookalike_as_the_docs_directory,
        it_fails_on_a_stray_markdown_file,
        it_keeps_the_repository_to_its_three_documents,
        it_flags_prose_streamed_into_an_assertion,
        it_flags_a_label_in_front_of_a_value,
        it_allows_a_streamed_value,
        it_allows_a_string_operand_with_no_stream,
        it_allows_prose_streamed_into_fail,
        it_allows_prose_streamed_into_add_failure,
        it_finds_a_stream_that_starts_on_a_later_line,
        it_finds_a_stream_after_a_multiline_argument_list,
        it_ignores_a_stream_belonging_to_the_next_statement,
        it_ignores_a_quote_inside_a_comment_after_the_assertion,
        it_ignores_a_shift_inside_the_streamed_expression,
        it_reports_the_line_the_assertion_starts_on,
        it_fails_on_streamed_prose,
        it_accepts_a_fixture_ending_in_test,
        it_flags_a_fixture_with_another_suffix,
        it_flags_a_fixture_that_leads_with_test,
        it_checks_fixtures_across_the_gtest_macros,
        it_reports_the_line_a_wrapped_fixture_sits_on,
        it_ignores_a_test_macro_inside_a_string_literal,
        it_leaves_every_real_fixture_name_alone,
        it_fails_on_a_fixture_without_the_suffix,
        it_flags_a_doxygen_block_on_a_struct,
        it_flags_a_block_separated_from_its_type_by_a_blank_line,
        it_looks_past_a_template_header_and_an_attribute,
        it_leaves_a_block_on_a_function_alone,
        it_leaves_a_block_on_a_data_member_alone,
        it_leaves_a_block_above_a_variable_of_a_class_type_alone,
        it_ignores_a_type_declared_inside_a_string_literal,
        it_fails_on_a_block_attached_to_a_type,
        it_accepts_a_mock_declared_with_gmock,
        it_accepts_a_hand_written_fake,
        it_flags_a_mock_without_gmock,
        it_flags_a_fake_built_with_gmock,
        it_judges_each_double_by_its_own_body,
        it_ignores_a_forward_declaration,
        it_ignores_a_name_that_merely_starts_with_the_letters,
        it_accepts_an_underscore_after_the_prefix,
        it_does_not_read_a_mock_method_out_of_a_neighbouring_double,
        it_ignores_a_mock_method_inside_a_string_literal,
        it_ignores_a_test_double_named_inside_a_string_literal,
        it_ignores_a_mock_method_inside_a_comment,
        it_keeps_a_masked_source_the_same_length_and_shape,
        it_fails_on_a_misnamed_test_double,
        it_accepts_the_canonical_include_order,
        it_flags_a_standard_header_before_the_own_header,
        it_flags_a_third_party_header_after_the_standard_block,
        it_allows_a_group_to_be_absent,
        it_reports_only_the_first_include_out_of_order,
        it_classifies_each_include_group,
        it_fails_on_an_include_out_of_order,
        it_ignores_includes_inside_a_preprocessor_conditional,
        it_stops_at_the_end_of_the_leading_include_block,
        it_allows_pragma_once_above_the_include_block,
        it_gates_dockerfiles_but_keeps_parser_directives,
        it_finds_violations_across_the_configured_globs,
        it_fails_by_default_and_only_warns_with_the_migration_flag,
        it_accepts_a_leaf_type_marked_final,
        it_flags_a_leaf_type_that_is_not_final,
        it_exempts_a_type_something_derives_from,
        it_exempts_a_gtest_fixture,
        it_exempts_a_double_wrapped_by_gmock,
        it_exempts_a_typed_suite,
        it_ignores_a_forward_declared_type,
        it_does_not_read_a_type_head_out_of_a_string,
        it_does_not_read_an_enum_as_a_type_head,
        it_reaches_a_nested_type,
        it_fails_on_a_type_that_is_not_final,
        it_fails_on_a_header_leading_with_test,
        it_allows_a_header_merely_containing_test,
        it_leaves_every_real_type_final_where_it_can_be,
        it_accepts_a_scoped_sized_enum,
        it_flags_a_scoped_enum_with_no_underlying_type,
        it_flags_an_unscoped_enum,
        it_reports_an_unscoped_enum_before_an_unsized_one,
        it_accepts_an_enum_struct,
        it_ignores_a_bare_forward_declaration,
        it_reads_a_sized_forward_declaration,
        it_does_not_read_an_enum_out_of_a_string_literal,
        it_fails_on_an_unsized_enum,
        it_leaves_every_real_enum_alone,
        it_flags_a_function_with_no_return_annotation,
        it_flags_a_parameter_with_no_annotation,
        it_accepts_a_fully_annotated_function,
        it_accepts_a_bare_self,
        it_wants_an_annotation_on_star_args,
        it_ignores_a_python_file_it_cannot_parse_for_annotations,
        it_fails_on_an_unannotated_function,
        it_leaves_every_real_python_function_annotated,
        it_accepts_a_prefixed_constant,
        it_flags_a_namespace_scope_constant_without_the_prefix,
        it_reads_a_brace_initialised_constant,
        it_leaves_a_constant_inside_a_body_alone,
        it_leaves_a_static_member_constant_alone,
        it_does_not_read_a_constexpr_function_as_a_constant,
        it_does_not_read_a_constexpr_declaration_as_a_constant,
        it_reads_a_variable_template,
        it_does_not_trip_on_a_templated_type,
        it_does_not_read_a_constant_out_of_a_string_literal,
        it_fails_on_a_constant_without_the_prefix,
        it_leaves_every_real_constant_alone,
        it_accepts_an_abstract_type_with_the_prefix,
        it_flags_an_abstract_type_without_the_prefix,
        it_leaves_a_type_with_a_virtual_body_alone,
        it_does_not_read_a_default_member_initialiser_as_pure,
        it_flags_an_abstract_class_template,
        it_reads_a_pure_virtual_carrying_specifiers,
        it_does_not_read_an_abstract_type_out_of_a_string_literal,
        it_does_not_flag_a_name_merely_starting_with_i,
        it_fails_on_an_abstract_type_without_the_prefix,
        it_leaves_every_real_abstract_type_alone,
        it_accepts_a_sorted_source_list,
        it_flags_a_source_list_out_of_order,
        it_reports_the_first_entry_out_of_place,
        it_leaves_a_single_entry_alone,
        it_checks_each_list_in_a_file_separately,
        it_does_not_join_two_lists_at_different_indents,
        it_does_not_read_a_keyword_as_a_source,
        it_fails_on_an_unsorted_source_list,
        it_leaves_every_real_source_list_sorted,
        it_accepts_a_header_named_for_its_one_type,
        it_flags_a_header_not_named_for_its_one_type,
        it_reports_the_line_the_type_is_declared_on,
        it_leaves_a_header_of_free_functions_alone,
        it_leaves_a_header_of_several_types_alone,
        it_leaves_a_header_holding_a_constant_alone,
        it_does_not_count_a_nested_type,
        it_accepts_an_enum_with_an_underlying_type,
        it_does_not_read_a_type_out_of_a_string_literal,
        it_ignores_a_forward_declaration_above_the_type,
        it_fails_on_a_header_not_named_for_its_type,
        it_leaves_every_real_header_name_alone,
        it_accepts_a_header_holding_one_struct,
        it_flags_the_second_struct_in_a_header,
        it_flags_every_type_after_the_first,
        it_leaves_a_nested_type_out_of_the_count,
        it_leaves_a_forward_declaration_out_of_the_count,
        it_lets_an_enum_share_a_header_with_a_struct,
        it_flags_a_crowded_header_through_find_violations,
        it_leaves_every_real_header_holding_one_shape,
        it_accepts_a_prefixed_double,
        it_flags_a_double_without_a_prefix,
        it_leaves_a_type_outside_a_tests_directory_alone,
        it_only_reads_a_base_that_looks_like_an_interface,
        it_reads_a_qualified_interface_base,
        it_does_not_read_a_double_out_of_a_string,
        it_fails_on_an_unprefixed_double,
        it_accepts_a_double_in_the_published_tree,
        it_flags_a_library_double_beside_its_tests,
        it_flags_a_library_double_declared_in_a_test_file,
        it_leaves_an_app_double_alone,
        it_leaves_a_backend_double_alone,
        it_does_not_read_a_double_out_of_a_string_literal,
        it_wants_the_module_directory_to_match_the_include_path,
        it_fails_on_a_library_double_outside_the_tree,
        it_leaves_every_real_double_where_it_is,
        it_accepts_a_typed_suite_ending_in_test,
        it_flags_a_typed_suite_without_the_suffix,
        it_reads_the_grammar_of_a_typed_test_name,
        it_reads_the_length_of_a_typed_test_name,
        it_exempts_a_phase_marker_in_a_typed_test_body,
        it_ignores_the_registration_and_instantiation_macros,
        it_flags_a_module_docstring,
        it_flags_a_function_docstring,
        it_flags_a_class_docstring,
        it_flags_a_docstring_written_with_one_quote,
        it_reports_the_line_a_wrapped_docstring_opens_on,
        it_ignores_a_triple_quoted_string_bound_to_a_name,
        it_ignores_a_string_that_is_not_the_first_statement,
        it_ignores_a_returned_string,
        it_ignores_a_python_file_it_cannot_parse,
        it_fails_on_a_python_docstring,
        it_leaves_every_script_free_of_docstrings,
        it_keeps_every_source_comment_within_the_style_guide,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
