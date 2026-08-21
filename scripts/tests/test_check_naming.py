#!/usr/bin/env python3

import importlib.util
import sys
import tempfile
from pathlib import Path

SCRIPT = Path(__file__).resolve().parent.parent / "check_naming.py"
DEFAULT_ROOT = SCRIPT.parent.parent

spec = importlib.util.spec_from_file_location("check_naming", SCRIPT)
check_naming = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check_naming)

m = check_naming


def write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)


def tree(source: str, name: str = "Thing.hpp") -> list:
    with tempfile.TemporaryDirectory() as raw:
        root = Path(raw)

        write(root / "src/libs/foo" / name, source)

        return m.find_violations(root)


def names(source: str, name: str = "Thing.hpp") -> list[str]:
    return [one.name for one in tree(source, name)]


def kinds(source: str, name: str = "Thing.hpp") -> list[str]:
    return [one.kind for one in tree(source, name)]


def it_reports_a_data_member_that_does_not_carry_its_type() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    class Holder final\n"
        "    {\n"
        "        foo::Bitmap sheet;\n"
        "    };\n"
        "}\n"
    )

    assert found == ["sheet"], found


def it_leaves_a_data_member_that_carries_its_type() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    class Holder final\n"
        "    {\n"
        "        foo::Bitmap editedBitmap;\n"
        "    };\n"
        "}\n"
    )

    assert found == [], found


def it_reports_a_parameter_that_does_not_carry_its_type() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint(foo::Bitmap sheet);\n"
        "}\n"
    )

    assert found == ["sheet"], found


def it_reports_a_parameter_of_a_definition() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void Holder::paint(const foo::Bitmap sheet)\n"
        "    {\n"
        "    }\n"
        "}\n",
        "Thing.cpp",
    )

    assert found == ["sheet"], found


def it_reports_a_local_that_does_not_carry_its_type() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void Holder::paint()\n"
        "    {\n"
        "        const foo::Bitmap sheet = read();\n"
        "    }\n"
        "}\n",
        "Thing.cpp",
    )

    assert found == ["sheet"], found


def it_reports_a_brace_initialised_local() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void Holder::paint()\n"
        "    {\n"
        "        const foo::Bitmap sheet{};\n"
        "    }\n"
        "}\n",
        "Thing.cpp",
    )

    assert found == ["sheet"], found


def it_reports_a_namespace_scope_constant() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    inline constexpr foo::Size kIconCell{16, 16};\n"
        "}\n"
    )

    assert found == ["kIconCell"], found


def it_leaves_a_constant_that_carries_its_type() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    inline constexpr foo::Size kIconSize{16, 16};\n"
        "}\n"
    )

    assert found == [], found


def it_reports_a_constant_without_the_k_prefix() -> None:
    found = tree(
        "namespace antwika::foo\n"
        "{\n"
        "    inline constexpr foo::Size iconSize{16, 16};\n"
        "}\n"
    )

    assert [one.name for one in found] == ["iconSize"], found
    assert found[0].kind == m.CONSTANT_SPELLING, found[0].kind


def it_looks_through_a_vector_to_its_element() -> None:
    assert m.carries_type("bitmaps", "std::vector<foo::Bitmap>")
    assert not m.carries_type("skins", "std::vector<foo::Bitmap>")


def it_accepts_a_plural_only_for_a_container() -> None:
    assert m.carries_type("bodies", "std::vector<Body>")
    assert not m.carries_type("bodies", "std::optional<Body>")
    assert m.carries_type("body", "std::optional<Body>")


def it_looks_through_a_unique_pointer_and_an_array() -> None:
    assert m.carries_type(
        "textures", "std::array<std::unique_ptr<gfx::ITexture>, 2>"
    )
    assert m.carries_type("sceneTarget", "std::unique_ptr<IRenderTarget>")


def it_looks_through_a_map_to_its_value() -> None:
    core, plural = m.unwrap("std::map<std::size_t, foo::Tile>")

    assert core == "foo::Tile", core
    assert plural


def it_strips_a_leading_i_from_an_interface() -> None:
    assert m.words("gfx::ITexture") == ["texture"]
    assert m.carries_type("texture", "gfx::ITexture *")


def it_keeps_every_word_of_a_two_word_type() -> None:
    assert m.words("ui::WidgetId") == ["widget", "id"]
    assert m.carries_type("kVariantWidget", "ui::WidgetId")


def it_leaves_a_vector_named_for_its_quantity() -> None:
    assert m.carries_type("position", "gfx::Vec3")
    assert m.carries_type("walkerPosition", "gfx::Vec3")
    assert m.carries_type("modelMatrix", "gfx::Mat4")
    assert m.carries_type("direction", "const gfx::Vec3 &")


def it_reports_a_vector_named_for_what_it_means() -> None:
    found = tree(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint(foo::Vec3 stood, foo::Mat4 model);\n"
        "}\n"
    )

    assert [one.name for one in found] == ["stood", "model"], found

    for one in found:
        assert one.kind == m.MATH_QUANTITY, one.kind


def it_leaves_a_builtin_and_a_std_type_alone() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    class Holder final\n"
        "    {\n"
        "        bool playing = false;\n"
        "        float viewHeight = 1.0F;\n"
        "        std::string mapPath;\n"
        "        std::size_t slotIndex = 0;\n"
        "    };\n"
        "}\n"
    )

    assert found == [], found


def it_reports_a_name_that_is_not_lower_camel_case() -> None:
    found = tree(
        "namespace antwika::foo\n"
        "{\n"
        "    void Holder::paint()\n"
        "    {\n"
        "        const foo::Bitmap was_lowest = read();\n"
        "    }\n"
        "}\n",
        "Thing.cpp",
    )

    assert [one.name for one in found] == ["was_lowest"], found
    assert found[0].kind == m.NOT_LOWER_CAMEL, found[0].kind


def it_reports_a_trailing_underscore() -> None:
    assert kinds(
        "namespace antwika::foo\n"
        "{\n"
        "    void Holder::paint()\n"
        "    {\n"
        "        const foo::Bitmap sheet_ = read();\n"
        "    }\n"
        "}\n",
        "Thing.cpp",
    ) == [m.NOT_LOWER_CAMEL]


def it_says_where_the_name_stands() -> None:
    found = tree(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint(foo::Bitmap sheet);\n"
        "}\n"
    )

    assert found[0].line == 3, found[0]
    assert found[0].column == 28, found[0]


def it_names_the_kind_a_role_parameter_falls_into() -> None:
    assert kinds(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint(foo::Bitmap sheet);\n"
        "}\n"
    ) == [m.ROLE_NAME]


def it_names_the_kind_an_index_falls_into() -> None:
    assert kinds(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint(foo::Bitmap at);\n"
        "}\n"
    ) == [m.INDEX_NAME]


def it_names_the_kind_an_accumulator_falls_into() -> None:
    assert kinds(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint(foo::Bitmap &out);\n"
        "}\n"
    ) == [m.ACCUMULATOR]


def it_names_the_kind_a_crowded_scope_falls_into() -> None:
    assert kinds(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint(foo::Bitmap from, foo::Bitmap to);\n"
        "}\n"
    ) == [m.CROWDED_SCOPE, m.CROWDED_SCOPE]


def it_names_the_kind_a_concept_plural_falls_into() -> None:
    assert kinds(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint(std::vector<foo::Bitmap> skins);\n"
        "}\n"
    ) == [m.CONCEPT_PLURAL]


def it_leaves_a_constructor_member_init_list_alone() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    Holder::Holder(foo::Bitmap given)\n"
        "        : sheet(given),\n"
        "          board{.width = 2, .height = 3},\n"
        "          count(0)\n"
        "    {\n"
        "    }\n"
        "}\n",
        "Thing.cpp",
    )

    assert found == ["given"], found


def it_leaves_a_mock_method_alone() -> None:
    assert names(
        "namespace antwika::foo::mocks\n"
        "{\n"
        "    class MockThing final : public IThing\n"
        "    {\n"
        "        MOCK_METHOD(void, paint, (const foo::Bitmap sheet),\n"
        "            (override));\n"
        "    };\n"
        "}\n"
    ) == []


def it_leaves_an_operator_alone() -> None:
    assert names(
        "namespace antwika::foo\n"
        "{\n"
        "    class Holder final\n"
        "    {\n"
        "        [[nodiscard]] bool operator==(const Holder &other)\n"
        "            const = default;\n"
        "        [[nodiscard]] foo::Bitmap *operator->() noexcept;\n"
        "    };\n"
        "}\n"
    ) == []


def it_leaves_a_cast_alone() -> None:
    assert names(
        "namespace antwika::foo\n"
        "{\n"
        "    void Holder::paint()\n"
        "    {\n"
        "        take(static_cast<std::size_t>(width));\n"
        "        take(reinterpret_cast<const char *>(bytes));\n"
        "    }\n"
        "}\n",
        "Thing.cpp",
    ) == []


def it_leaves_a_structured_binding_alone() -> None:
    assert names(
        "namespace antwika::foo\n"
        "{\n"
        "    void Holder::paint()\n"
        "    {\n"
        "        const auto [cell, value] = entries.back();\n"
        "    }\n"
        "}\n",
        "Thing.cpp",
    ) == []


def it_leaves_a_range_for_alone() -> None:
    assert names(
        "namespace antwika::foo\n"
        "{\n"
        "    void Holder::paint()\n"
        "    {\n"
        "        for (const foo::Bitmap &one : sheets)\n"
        "        {\n"
        "        }\n"
        "    }\n"
        "}\n",
        "Thing.cpp",
    ) == []


def it_leaves_a_catch_clause_alone() -> None:
    assert names(
        "namespace antwika::foo\n"
        "{\n"
        "    void Holder::paint()\n"
        "    {\n"
        "        try\n"
        "        {\n"
        "            read();\n"
        "        }\n"
        "        catch (const foo::ReadError &given)\n"
        "        {\n"
        "        }\n"
        "    }\n"
        "}\n",
        "Thing.cpp",
    ) == []


def it_leaves_a_throw_alone() -> None:
    assert names(
        "namespace antwika::foo\n"
        "{\n"
        "    void Holder::paint()\n"
        "    {\n"
        "        throw foo::ReadError(\"no\");\n"
        "    }\n"
        "}\n",
        "Thing.cpp",
    ) == []


def it_leaves_a_function_pointer_alone() -> None:
    assert names(
        "namespace antwika::foo\n"
        "{\n"
        "    class Holder final\n"
        "    {\n"
        "        foo::Bitmap *(*setUp)(foo::Bitmap *) = nullptr;\n"
        "    };\n"
        "}\n"
    ) == []


def it_leaves_an_unnamed_parameter_alone() -> None:
    assert names(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint(foo::Bitmap &, const foo::Size);\n"
        "}\n"
    ) == []


def it_leaves_a_requires_clause_alone() -> None:
    assert names(
        "namespace antwika::foo\n"
        "{\n"
        "    template <typename OverlayT>\n"
        "    concept HasDrawList = requires(const OverlayT &overlay)\n"
        "    {\n"
        "        { overlay.commands() } -> std::convertible_to<int>;\n"
        "    };\n"
        "}\n"
    ) == []


def it_leaves_a_forward_declaration_and_an_alias_alone() -> None:
    assert names(
        "namespace antwika::foo\n"
        "{\n"
        "    class LayoutTree;\n"
        "    using Painted = foo::Bitmap;\n"
        "    using antwika::gfx::Color;\n"
        "}\n"
    ) == []


def it_leaves_a_comment_and_a_string_alone() -> None:
    assert names(
        "namespace antwika::foo\n"
        "{\n"
        "    void Holder::paint()\n"
        "    {\n"
        "        say(\"foo::Bitmap sheet\");\n"
        "    }\n"
        "}\n",
        "Thing.cpp",
    ) == []


def it_leaves_an_enumerator_alone() -> None:
    assert names(
        "namespace antwika::foo\n"
        "{\n"
        "    enum class Kind : std::uint8_t\n"
        "    {\n"
        "        Normal = 0,\n"
        "        Water = 1,\n"
        "    };\n"
        "}\n"
    ) == []


def it_leaves_a_third_party_type_alone() -> None:
    assert names(
        "namespace antwika::foo\n"
        "{\n"
        "    class Holder final\n"
        "    {\n"
        "        ::Texture2D texture{};\n"
        "        nlohmann::json document;\n"
        "    };\n"
        "}\n"
    ) == []


def it_reads_an_unqualified_type_the_tree_declares() -> None:
    assert names(
        "namespace antwika::foo\n"
        "{\n"
        "    struct Bitmap final\n"
        "    {\n"
        "        std::size_t width = 0;\n"
        "    };\n"
        "\n"
        "    void paint(Bitmap sheet);\n"
        "}\n"
    ) == ["sheet"]


def it_finds_nothing_wrong_in_the_real_tree() -> None:
    violations = m.find_violations(DEFAULT_ROOT)
    said = [
        f"{one.path}:{one.line}:{one.column}: {one.name}"
        for one in violations
    ]

    assert not violations, "\n".join(said[:10])


def it_puts_every_reported_column_on_the_name() -> None:
    sources: dict[Path, list[str]] = {}
    wrong = []

    for one in m.find_violations(DEFAULT_ROOT):
        lines = sources.setdefault(
            one.path,
            one.path.read_text(encoding="utf-8", errors="ignore").split("\n"),
        )
        found = lines[one.line - 1][one.column - 1:][:len(one.name)]

        if found != one.name:
            wrong.append(f"{one.path}:{one.line}:{one.column}: {one.name}")

    assert not wrong, "\n".join(wrong[:10])


def it_leaves_the_names_that_already_carry_their_type() -> None:
    reported = {
        (one.path.name, one.name) for one in m.find_violations(DEFAULT_ROOT)
    }
    kept = (
        ("AtlasSheets.hpp", "bitmaps"),
        ("ScenePass.hpp", "sceneTarget"),
        ("ScenePass.hpp", "screenMesh"),
        ("Editor.hpp", "history"),
        ("WorldMeshes.hpp", "solidMesh"),
        ("Sprites.hpp", "figureMesh"),
    )

    for one in kept:
        assert one not in reported, one


def it_leaves_the_doubles_and_the_macros_silent() -> None:
    quiet = (
        "src/libs/gfx/tests/mocks/include/antwika/gfx/mocks/"
        "MockGfxBackend.hpp",
        "src/libs/log/tests/mocks/include/antwika/log/mocks/MockLogger.hpp",
    )
    reported = {one.path for one in m.find_violations(DEFAULT_ROOT)}

    for one in quiet:
        assert DEFAULT_ROOT / one not in reported, one


def it_reports_a_local_named_for_a_preposition() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint()\n"
        "    {\n"
        "        const auto out = 1U;\n"
        "    }\n"
        "}\n"
    )

    assert found == ["out"], found


def it_reports_a_local_named_for_a_participle() -> None:
    found = kinds(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint()\n"
        "    {\n"
        "        const auto stood = 1U;\n"
        "    }\n"
        "}\n"
    )

    assert found == [m.PART_OF_SPEECH], found


def it_reports_a_local_named_for_a_bare_adjective() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint()\n"
        "    {\n"
        "        const auto whole = 1U;\n"
        "    }\n"
        "}\n"
    )

    assert found == ["whole"], found


def it_leaves_a_bool_named_for_a_participle_alone() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint()\n"
        "    {\n"
        "        const bool climbed = true;\n"
        "    }\n"
        "}\n"
    )

    assert found == [], found


def it_leaves_a_compound_name_ending_in_a_noun_alone() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint()\n"
        "    {\n"
        "        const auto stoodPosition = 1U;\n"
        "    }\n"
        "}\n"
    )

    assert found == [], found


def it_leaves_a_noun_that_merely_ends_in_ed_alone() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint()\n"
        "    {\n"
        "        const auto speed = 1U;\n"
        "    }\n"
        "}\n"
    )

    assert found == [], found


def it_leaves_a_name_that_carries_its_type_alone() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    struct Run final\n"
        "    {\n"
        "        int value{};\n"
        "    };\n"
        "\n"
        "    void paint()\n"
        "    {\n"
        "        const foo::Run run{};\n"
        "    }\n"
        "}\n"
    )

    assert found == [], found


def it_reports_a_range_for_variable() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint(const std::vector<std::size_t> &values)\n"
        "    {\n"
        "        for (const auto stood : values)\n"
        "        {\n"
        "            (void)stood;\n"
        "        }\n"
        "    }\n"
        "}\n"
    )

    assert found == ["stood"], found


def it_reports_a_for_init_variable() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint()\n"
        "    {\n"
        "        for (std::size_t out = 0; out < 2U; ++out)\n"
        "        {\n"
        "        }\n"
        "    }\n"
        "}\n"
    )

    assert found == ["out"], found


def it_reports_a_condition_init_variable() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint(const std::vector<std::size_t> &values)\n"
        "    {\n"
        "        if (const auto found = values.size(); found > 0U)\n"
        "        {\n"
        "        }\n"
        "    }\n"
        "}\n"
    )

    assert found == ["found"], found


def it_reports_a_structured_binding() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint()\n"
        "    {\n"
        "        const auto [first, drawn] = std::pair{1U, 2U};\n"
        "    }\n"
        "}\n"
    )

    assert found == ["drawn"], found


def it_reports_a_lambda_parameter() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint()\n"
        "    {\n"
        "        const auto adder = [](std::size_t whole) { return whole; };\n"
        "    }\n"
        "}\n"
    )

    assert found == ["whole"], found


def it_leaves_a_noun_in_a_stepped_over_place_alone() -> None:
    found = names(
        "namespace antwika::foo\n"
        "{\n"
        "    void paint(const std::vector<std::size_t> &values)\n"
        "    {\n"
        "        for (const auto slotIndex : values)\n"
        "        {\n"
        "            (void)slotIndex;\n"
        "        }\n"
        "    }\n"
        "}\n"
    )

    assert found == [], found


def main() -> int:
    tests = [
        it_reports_a_data_member_that_does_not_carry_its_type,
        it_leaves_a_data_member_that_carries_its_type,
        it_reports_a_parameter_that_does_not_carry_its_type,
        it_reports_a_parameter_of_a_definition,
        it_reports_a_local_that_does_not_carry_its_type,
        it_reports_a_brace_initialised_local,
        it_reports_a_namespace_scope_constant,
        it_leaves_a_constant_that_carries_its_type,
        it_reports_a_constant_without_the_k_prefix,
        it_looks_through_a_vector_to_its_element,
        it_accepts_a_plural_only_for_a_container,
        it_looks_through_a_unique_pointer_and_an_array,
        it_looks_through_a_map_to_its_value,
        it_strips_a_leading_i_from_an_interface,
        it_keeps_every_word_of_a_two_word_type,
        it_leaves_a_vector_named_for_its_quantity,
        it_reports_a_vector_named_for_what_it_means,
        it_leaves_a_builtin_and_a_std_type_alone,
        it_reports_a_name_that_is_not_lower_camel_case,
        it_reports_a_trailing_underscore,
        it_says_where_the_name_stands,
        it_names_the_kind_a_role_parameter_falls_into,
        it_names_the_kind_an_index_falls_into,
        it_names_the_kind_an_accumulator_falls_into,
        it_names_the_kind_a_crowded_scope_falls_into,
        it_names_the_kind_a_concept_plural_falls_into,
        it_leaves_a_constructor_member_init_list_alone,
        it_leaves_a_mock_method_alone,
        it_leaves_an_operator_alone,
        it_leaves_a_cast_alone,
        it_leaves_a_structured_binding_alone,
        it_leaves_a_range_for_alone,
        it_leaves_a_catch_clause_alone,
        it_leaves_a_throw_alone,
        it_leaves_a_function_pointer_alone,
        it_leaves_an_unnamed_parameter_alone,
        it_leaves_a_requires_clause_alone,
        it_leaves_a_forward_declaration_and_an_alias_alone,
        it_leaves_a_comment_and_a_string_alone,
        it_leaves_an_enumerator_alone,
        it_leaves_a_third_party_type_alone,
        it_reads_an_unqualified_type_the_tree_declares,
        it_finds_nothing_wrong_in_the_real_tree,
        it_puts_every_reported_column_on_the_name,
        it_leaves_the_names_that_already_carry_their_type,
        it_leaves_the_doubles_and_the_macros_silent,
        it_reports_a_local_named_for_a_preposition,
        it_reports_a_local_named_for_a_participle,
        it_reports_a_local_named_for_a_bare_adjective,
        it_leaves_a_bool_named_for_a_participle_alone,
        it_leaves_a_compound_name_ending_in_a_noun_alone,
        it_leaves_a_noun_that_merely_ends_in_ed_alone,
        it_leaves_a_name_that_carries_its_type_alone,
        it_reports_a_range_for_variable,
        it_reports_a_for_init_variable,
        it_reports_a_condition_init_variable,
        it_reports_a_structured_binding,
        it_reports_a_lambda_parameter,
        it_leaves_a_noun_in_a_stepped_over_place_alone,
    ]

    for test in tests:
        test()
        print(f"OK: {test.__name__}")

    print(f"{len(tests)} tests passed")

    return 0


if __name__ == "__main__":
    sys.exit(main())
