#include <gtest/gtest.h>

#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/ScrollChange.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::ui::Interactions;
using antwika::ui::kNoWidget;
using antwika::ui::OptionChoice;
using antwika::ui::ScrollChange;
using antwika::ui::TextAreaSpec;
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
}

TEST(FieldValueTest, Ctor_DefaultsATextEditToNothing)
{
    const TextEdit edit{};

    EXPECT_EQ(kNoWidget, edit.field);
    EXPECT_EQ("", edit.text);
    EXPECT_EQ(0U, edit.cursor);
    EXPECT_FALSE(edit.submitted);
    EXPECT_FALSE(edit.cancelled);
}

TEST(FieldValueTest, Ctor_DefaultsAFieldToEmptyAndUnfocused)
{
    const TextFieldSpec spec{};

    EXPECT_EQ(kNoWidget, spec.id);
    EXPECT_TRUE(spec.text.empty());
    EXPECT_TRUE(spec.placeholder.empty());
    EXPECT_FALSE(spec.focused);
}

TEST(FieldValueTest, OperatorEquals_MatchesTextEditsOnEveryField)
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

    auto selected = anEdit();
    selected.anchor = 1;
    EXPECT_NE(anEdit(), selected);

    auto copied = anEdit();
    copied.copied = "ab";
    EXPECT_NE(anEdit(), copied);

    auto submitted = anEdit();
    submitted.submitted = true;
    EXPECT_NE(anEdit(), submitted);

    auto cancelled = anEdit();
    cancelled.cancelled = true;
    EXPECT_NE(anEdit(), cancelled);
}

TEST(FieldValueTest, OperatorEquals_MatchesChoicesOnBothParts)
{
    constexpr OptionChoice choice{.dropdown = kField, .index = 1};

    EXPECT_EQ(choice, (OptionChoice{.dropdown = kField, .index = 1}));
    EXPECT_NE(
        choice, (OptionChoice{.dropdown = WidgetId{4}, .index = 1}));
    EXPECT_NE(choice, (OptionChoice{.dropdown = kField, .index = 2}));
}

TEST(FieldValueTest, OperatorEquals_ComparesTheEditAndTheChoice)
{
    const Interactions quiet{};

    EXPECT_EQ(quiet, Interactions{});

    Interactions edited;
    edited.edit = anEdit();
    EXPECT_NE(quiet, edited);

    Interactions chose;
    chose.chosen = OptionChoice{.dropdown = kField, .index = 0};
    EXPECT_NE(quiet, chose);

    Interactions scrolled;
    scrolled.scrolled = ScrollChange{.area = kField, .line = 2};
    EXPECT_NE(quiet, scrolled);
}

TEST(FieldValueTest, OperatorEquals_MatchesScrollReportsOnBothParts)
{
    constexpr ScrollChange showing{.area = kField, .line = 3};

    EXPECT_EQ(showing, (ScrollChange{.area = kField, .line = 3}));
    EXPECT_NE(showing, (ScrollChange{.area = WidgetId{4}, .line = 3}));
    EXPECT_NE(showing, (ScrollChange{.area = kField, .line = 4}));
}

TEST(FieldValueTest, Ctor_DefaultsToNoSelectionOrCopy)
{
    const TextEdit edit{};

    EXPECT_EQ(edit.anchor, edit.cursor);
    EXPECT_TRUE(edit.copied.empty());
}

TEST(FieldValueTest, Ctor_DefaultsAnAreaToNoSelectionOrBar)
{
    const TextAreaSpec spec{};

    EXPECT_FALSE(spec.anchor.has_value());
    EXPECT_EQ(spec.scroll, 0U);
    EXPECT_FALSE(spec.scrollbar);
}
