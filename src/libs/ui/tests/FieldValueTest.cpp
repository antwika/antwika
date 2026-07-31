#include <gtest/gtest.h>

#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::ui::Interactions;
using antwika::ui::kNoWidget;
using antwika::ui::OptionChoice;
using antwika::ui::TextEdit;
using antwika::ui::TextFieldSpec;
using antwika::ui::WidgetId;

namespace
{
    constexpr WidgetId kField{3};

    [[nodiscard]] TextEdit anEdit()
    {
        return TextEdit{
            .field = kField,
            .text = "ab",
            .cursor = 2,
            .submitted = false,
            .cancelled = false};
    }
} // namespace

TEST(FieldValueTest, ATextEditDefaultsToNothingHavingHappened)
{
    const TextEdit edit{};

    EXPECT_EQ(kNoWidget, edit.field);
    EXPECT_EQ("", edit.text);
    EXPECT_EQ(0U, edit.cursor);
    EXPECT_FALSE(edit.submitted);
    EXPECT_FALSE(edit.cancelled);
}

TEST(FieldValueTest, ATextFieldDefaultsToAnEmptyUnfocusedField)
{
    const TextFieldSpec spec{};

    EXPECT_EQ(kNoWidget, spec.id);
    EXPECT_TRUE(spec.text.empty());
    EXPECT_TRUE(spec.placeholder.empty());
    EXPECT_FALSE(spec.focused);
}

TEST(FieldValueTest, TwoTextEditsMatchOnlyWhenEveryFieldDoes)
{
    EXPECT_EQ(anEdit(), anEdit());

    auto named = anEdit();
    named.field = WidgetId{4};
    EXPECT_NE(anEdit(), named);

    auto written = anEdit();
    written.text = "abc";
    EXPECT_NE(anEdit(), written);

    auto moved = anEdit();
    moved.cursor = 1;
    EXPECT_NE(anEdit(), moved);

    auto submitted = anEdit();
    submitted.submitted = true;
    EXPECT_NE(anEdit(), submitted);

    auto cancelled = anEdit();
    cancelled.cancelled = true;
    EXPECT_NE(anEdit(), cancelled);
}

TEST(FieldValueTest, TwoChoicesMatchOnlyWhenBothPartsDo)
{
    constexpr OptionChoice choice{.dropdown = kField, .index = 1};

    EXPECT_EQ(choice, (OptionChoice{.dropdown = kField, .index = 1}));
    EXPECT_NE(
        choice, (OptionChoice{.dropdown = WidgetId{4}, .index = 1}));
    EXPECT_NE(choice, (OptionChoice{.dropdown = kField, .index = 2}));
}

TEST(FieldValueTest, InteractionsCompareTheirEditAndTheirChoiceToo)
{
    const Interactions quiet{};

    EXPECT_EQ(quiet, Interactions{});

    Interactions edited;
    edited.edit = anEdit();
    EXPECT_NE(quiet, edited);

    Interactions chose;
    chose.chosen = OptionChoice{.dropdown = kField, .index = 0};
    EXPECT_NE(quiet, chose);
}
