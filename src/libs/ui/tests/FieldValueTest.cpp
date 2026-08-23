#include <gtest/gtest.h>

#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/ScrollChange.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::ui::Interactions;
using antwika::widget::kNoWidget;
using antwika::ui::OptionChoice;
using antwika::ui::ScrollChange;
using antwika::ui::TextAreaSpec;
using antwika::ui::TextEdit;
using antwika::ui::TextFieldSpec;
using antwika::widget::WidgetId;

namespace
{
    constexpr WidgetId kFieldWidget{3};

    [[nodiscard]] TextEdit getAnEdit()
    {
        return TextEdit{
            .fieldWidget = kFieldWidget,
            .text = "ab",
            .cursor = 2,
            .submitted = false,
            .cancelled = false};
    }
}

TEST(FieldValueTest, Ctor_DefaultsATextEditToNothing)
{
    const TextEdit edit{};

    EXPECT_EQ(kNoWidget, edit.fieldWidget);
    EXPECT_EQ("", edit.text);
    EXPECT_EQ(0U, edit.cursor);
    EXPECT_FALSE(edit.submitted);
    EXPECT_FALSE(edit.cancelled);
}

TEST(FieldValueTest, Ctor_DefaultsAFieldToEmptyAndUnfocused)
{
    const TextFieldSpec spec{};

    EXPECT_EQ(kNoWidget, spec.widgetId);
    EXPECT_TRUE(spec.text.empty());
    EXPECT_TRUE(spec.placeholder.empty());
    EXPECT_FALSE(spec.focused);
}

TEST(FieldValueTest, OperatorEquals_MatchesTextEditsOnEveryField)
{
    EXPECT_EQ(getAnEdit(), getAnEdit());

    auto edit = getAnEdit();
    edit.fieldWidget = WidgetId{4};
    EXPECT_NE(getAnEdit(), edit);

    auto typedEdit = getAnEdit();
    typedEdit.text = "abc";
    EXPECT_NE(getAnEdit(), typedEdit);

    auto movedEdit = getAnEdit();
    movedEdit.cursor = 1;
    EXPECT_NE(getAnEdit(), movedEdit);

    auto selectedEdit = getAnEdit();
    selectedEdit.anchor = 1;
    EXPECT_NE(getAnEdit(), selectedEdit);

    auto copiedEdit = getAnEdit();
    copiedEdit.copiedText = "ab";
    EXPECT_NE(getAnEdit(), copiedEdit);

    auto submittedEdit = getAnEdit();
    submittedEdit.submitted = true;
    EXPECT_NE(getAnEdit(), submittedEdit);

    auto cancelledEdit = getAnEdit();
    cancelledEdit.cancelled = true;
    EXPECT_NE(getAnEdit(), cancelledEdit);
}

TEST(FieldValueTest, OperatorEquals_MatchesChoicesOnBothParts)
{
    constexpr OptionChoice choice{.dropdownWidget = kFieldWidget, .index = 1};

    EXPECT_EQ(
        choice,
        (OptionChoice{.dropdownWidget = kFieldWidget, .index = 1}));
    EXPECT_NE(
        choice, (OptionChoice{.dropdownWidget = WidgetId{4}, .index = 1}));
    EXPECT_NE(
        choice,
        (OptionChoice{.dropdownWidget = kFieldWidget, .index = 2}));
}

TEST(FieldValueTest, OperatorEquals_ComparesTheEditAndTheChoice)
{
    const Interactions quietInteractions{};

    EXPECT_EQ(quietInteractions, Interactions{});

    Interactions editedInteractions;
    editedInteractions.edit = getAnEdit();
    EXPECT_NE(quietInteractions, editedInteractions);

    Interactions choseInteractions;
    choseInteractions.chosenChoice =
        OptionChoice{.dropdownWidget = kFieldWidget, .index = 0};
    EXPECT_NE(quietInteractions, choseInteractions);

    Interactions scrolledInteractions;
    scrolledInteractions.scrollChange =
        ScrollChange{.areaWidget = kFieldWidget, .line = 2};
    EXPECT_NE(quietInteractions, scrolledInteractions);
}

TEST(FieldValueTest, OperatorEquals_MatchesScrollReportsOnBothParts)
{
    constexpr ScrollChange showingChange{.areaWidget = kFieldWidget, .line = 3};

    EXPECT_EQ(
        showingChange,
        (
            ScrollChange{.areaWidget = kFieldWidget, .line = 3}));
    EXPECT_NE(
        showingChange,
        (ScrollChange{.areaWidget = WidgetId{4}, .line = 3}));
    EXPECT_NE(
        showingChange,
        (ScrollChange{.areaWidget = kFieldWidget, .line = 4}));
}

TEST(FieldValueTest, Ctor_DefaultsToNoSelectionOrCopy)
{
    const TextEdit edit{};

    EXPECT_EQ(edit.anchor, edit.cursor);
    EXPECT_TRUE(edit.copiedText.empty());
}

TEST(FieldValueTest, Ctor_DefaultsAnAreaToNoSelectionOrBar)
{
    const TextAreaSpec spec{};

    EXPECT_FALSE(spec.anchor.has_value());
    EXPECT_EQ(spec.scroll, 0U);
    EXPECT_FALSE(spec.scrollbar);
}
