#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/support/DrawListQueries.hpp>

#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Frame.hpp"
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::ui::support::getFillsColored;
using antwika::ui::support::textsOf;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::Frame;
using antwika::ui::Key;
using antwika::ui::Keyboard;
using antwika::ui::Pointer;
using antwika::ui::TextAreaSpec;
using antwika::ui::TextEdit;
using antwika::ui::Theme;
using antwika::widget::WidgetId;

namespace
{
    constexpr Color kInkColor{.red = 200, .green = 210, .blue = 220};
    constexpr Color kPickedColor{.red = 40, .green = 70, .blue = 120};

    constexpr WidgetId kCodeWidget{9};

    constexpr Size kCanvasSize{.width = 200, .height = 100};

    constexpr std::int32_t kAdvance = 6;
    constexpr std::int32_t kLineHeight = 8;

    Theme getPlainTheme()
    {
        return Theme{
            .textColor = kInkColor,
            .selectionColor = kPickedColor,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    [[nodiscard]] Frame frameOf(
        TextAreaSpec spec,
        const Keyboard &keyboard = {},
        const Pointer &pointer = {})
    {
        spec.widgetId = kCodeWidget;
        spec.focused = true;

        Context uiContext{kCanvasSize, getPlainTheme(), pointer, keyboard};

        uiContext.textArea(spec);

        return uiContext.build();
    }

    [[nodiscard]] TextEdit editOf(
        TextAreaSpec spec,
        const Keyboard &keyboard = {},
        const Pointer &pointer = {})
    {
        const auto frame = frameOf(spec, keyboard, pointer);

        EXPECT_TRUE(frame.interactions.edit.has_value());

        return frame.interactions.edit.value_or(TextEdit{});
    }

    [[nodiscard]] std::vector<std::string> pickedOf(
        const DrawList &drawList)
    {
        std::vector<std::string> pickedTexts;
        std::optional<FillRect> groundRect;

        for (const auto &command : drawList)
        {
            if (const auto *fill = std::get_if<FillRect>(&command))
            {
                groundRect = fill->color == kPickedColor
                           ? std::optional{*fill}
                           : std::nullopt;

                continue;
            }

            const auto *text = std::get_if<DrawText>(&command);

            if (text != nullptr && groundRect.has_value()
                && groundRect->rect.originPoint == text->originPoint)
            {
                pickedTexts.push_back(text->text);
            }

            groundRect.reset();
        }

        return pickedTexts;
    }

    [[nodiscard]] std::size_t groundsOf(const DrawList &drawList)
    {
        std::size_t grounds = 0;

        for (const auto &command : drawList)
        {
            const auto *fill = std::get_if<FillRect>(&command);

            if (fill != nullptr && fill->color == kPickedColor)
            {
                ++grounds;
            }
        }

        return grounds;
    }

    [[nodiscard]] Point cellAt(std::int32_t line, std::int32_t column)
    {
        return Point{
            .x = column * kAdvance + 1, .y = line * kLineHeight + 1};
    }
}

TEST(TextAreaSelectionTest, TextArea_AnAreaToldNothingHasNothingSelected)
{
    const auto picture =
        frameOf(TextAreaSpec{.text = "abcd", .cursor = 2}).drawList;

    ASSERT_EQ(textsOf(picture), (std::vector<std::string>{"ab", "cd"}));
    EXPECT_EQ(groundsOf(picture), 0U);
}

TEST(TextAreaSelectionTest, TextArea_DrawsTheSelectedCharactersOnTheirOwnGround)
{
    const auto picture =
        frameOf(TextAreaSpec{.text = "abcdef", .cursor = 4, .anchor = 1})
            .drawList;

    EXPECT_EQ(pickedOf(picture), std::vector<std::string>{"bcd"});
}

TEST(TextAreaSelectionTest, TextArea_TheEndsAreInterchangeable)
{
    const auto forwards =
        frameOf(TextAreaSpec{.text = "abcdef", .cursor = 4, .anchor = 1});

    const auto backwards =
        frameOf(TextAreaSpec{.text = "abcdef", .cursor = 1, .anchor = 4});

    EXPECT_EQ(
        pickedOf(forwards.drawList), std::vector<std::string>{"bcd"});
    EXPECT_EQ(
        pickedOf(backwards.drawList), pickedOf(forwards.drawList));
}

TEST(TextAreaSelectionTest, TextArea_ASelectionAcrossLinesIsDrawnOnEachOfThem)
{
    const auto picture =
        frameOf(TextAreaSpec{
                    .text = "ab\n\ncd", .cursor = 5, .anchor = 1})
            .drawList;

    const std::vector<std::string> expectedLines{"b", "c"};

    EXPECT_EQ(pickedOf(picture), expectedLines);

    EXPECT_EQ(groundsOf(picture), 4U);
}

TEST(TextAreaSelectionTest, TextArea_AnUnfocusedAreaShowsNoSelection)
{
    Context uiContext{kCanvasSize, getPlainTheme()};

    uiContext.textArea(
        TextAreaSpec{.text = "abcdef", .cursor = 4, .anchor = 1});

    const auto picture = uiContext.build().drawList;

    ASSERT_EQ(textsOf(picture), (std::vector<std::string>{"abcdef"}));
    EXPECT_EQ(groundsOf(picture), 0U);
}

TEST(TextAreaSelectionTest, TextArea_ShiftAndAnArrowLeavesTheFarEndWhereItIs)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcd", .cursor = 1},
        Keyboard{.keys = {Key::SelectRight, Key::SelectRight}});

    EXPECT_EQ(edit.cursor, 3U);
    EXPECT_EQ(edit.anchor, 1U);
}

TEST(TextAreaSelectionTest, TextArea_ShiftAndTheLeftArrowSelectsBackwards)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcd", .cursor = 3},
        Keyboard{.keys = {Key::SelectLeft, Key::SelectLeft}});

    EXPECT_EQ(edit.cursor, 1U);
    EXPECT_EQ(edit.anchor, 3U);
}

TEST(TextAreaSelectionTest, TextArea_SelectingUpAndDownWalksTheLines)
{
    const auto downEdit = editOf(
        TextAreaSpec{.text = "abcd\nefgh", .cursor = 1},
        Keyboard{.keys = {Key::SelectDown}});

    EXPECT_EQ(downEdit.cursor, 6U);
    EXPECT_EQ(downEdit.anchor, 1U);

    const auto movedUp = editOf(
        TextAreaSpec{.text = "abcd\nefgh", .cursor = 6},
        Keyboard{.keys = {Key::SelectUp}});

    EXPECT_EQ(movedUp.cursor, 1U);
    EXPECT_EQ(movedUp.anchor, 6U);
}

TEST(TextAreaSelectionTest, TextArea_SelectingPastEitherEndDoesNothing)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = "ab", .cursor = 0},
        Keyboard{.keys = {Key::SelectLeft, Key::SelectUp}});

    EXPECT_FALSE(frame.interactions.edit.has_value());

    const auto secondFrame = frameOf(
        TextAreaSpec{.text = "ab", .cursor = 2},
        Keyboard{.keys = {Key::SelectRight, Key::SelectDown}});

    EXPECT_FALSE(secondFrame.interactions.edit.has_value());
}

TEST(TextAreaSelectionTest, TextArea_APlainMoveCollapsesToTheEndItIsHeadingFor)
{
    const auto left = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 4, .anchor = 1},
        Keyboard{.keys = {Key::MoveLeft}});

    EXPECT_EQ(left.cursor, 1U);
    EXPECT_EQ(left.anchor, 1U);

    const auto right = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1, .anchor = 4},
        Keyboard{.keys = {Key::MoveRight}});

    EXPECT_EQ(right.cursor, 4U);
    EXPECT_EQ(right.anchor, 4U);
}

TEST(TextAreaSelectionTest, TextArea_MovingUpOrDownCollapsesTheSelectionToo)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcd\nefgh", .cursor = 7, .anchor = 9},
        Keyboard{.keys = {Key::MoveUp}});

    EXPECT_EQ(edit.cursor, 2U);
    EXPECT_EQ(edit.anchor, 2U);
}

TEST(TextAreaSelectionTest, TextArea_HomeAndEndCollapseTheSelectionToo)
{
    const auto home = editOf(
        TextAreaSpec{.text = "abcd\nefgh", .cursor = 7, .anchor = 2},
        Keyboard{.keys = {Key::MoveLineStart}});

    EXPECT_EQ(home.cursor, 5U);
    EXPECT_EQ(home.anchor, 5U);

    const auto end = editOf(
        TextAreaSpec{.text = "abcd\nefgh", .cursor = 2, .anchor = 7},
        Keyboard{.keys = {Key::MoveLineEnd}});

    EXPECT_EQ(end.cursor, 4U);
    EXPECT_EQ(end.anchor, 4U);
}

TEST(TextAreaSelectionTest, TextArea_ShiftedHomeAndEndSelectToTheLinesEnds)
{
    const auto home = editOf(
        TextAreaSpec{.text = "abcd\nefgh", .cursor = 7},
        Keyboard{.keys = {Key::SelectLineStart}});

    EXPECT_EQ(home.cursor, 5U);
    EXPECT_EQ(home.anchor, 7U);

    const auto end = editOf(
        TextAreaSpec{.text = "abcd\nefgh", .cursor = 6},
        Keyboard{.keys = {Key::SelectLineEnd}});

    EXPECT_EQ(end.cursor, 9U);
    EXPECT_EQ(end.anchor, 6U);
}

TEST(TextAreaSelectionTest, TextArea_TypingOverASelectionTakesTheWholeOfIt)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1, .anchor = 4},
        Keyboard{.keys = {Key::Character}, .typedText = "X"});

    EXPECT_EQ(edit.text, "aXef");
    EXPECT_EQ(edit.cursor, 2U);
    EXPECT_EQ(edit.anchor, 2U);
}

TEST(TextAreaSelectionTest, TextArea_EnterOverASelectionReplacesItWithABreak)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1, .anchor = 4},
        Keyboard{.keys = {Key::Activate}});

    EXPECT_EQ(edit.text, "a\nef");
}

TEST(TextAreaSelectionTest, TextArea_BackspaceAndDeleteBothTakeAWholeSelection)
{
    const auto movedEdit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1, .anchor = 4},
        Keyboard{.keys = {Key::Backspace}});

    EXPECT_EQ(movedEdit.text, "aef");
    EXPECT_EQ(movedEdit.cursor, 1U);

    const auto forward = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 4, .anchor = 1},
        Keyboard{.keys = {Key::Delete}});

    EXPECT_EQ(forward.text, "aef");
    EXPECT_EQ(forward.cursor, 1U);
}

TEST(TextAreaSelectionTest, TextArea_DeleteTakesTheCharacterTheCaretSitsBefore)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abc", .cursor = 1},
        Keyboard{.keys = {Key::Delete}});

    EXPECT_EQ(edit.text, "ac");
    EXPECT_EQ(edit.cursor, 1U);
}

TEST(TextAreaSelectionTest, TextArea_DeleteAtTheEndHasNothingToTake)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = "abc", .cursor = 3},
        Keyboard{.keys = {Key::Delete}});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaSelectionTest, TextArea_SelectAllReachesFromEndToEnd)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcd\nefgh", .cursor = 2},
        Keyboard{.keys = {Key::SelectAll}});

    EXPECT_EQ(edit.anchor, 0U);
    EXPECT_EQ(edit.cursor, edit.text.size());
}

TEST(TextAreaSelectionTest, TextArea_SelectAllThenCopyTakesTheWholeText)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 2},
        Keyboard{.keys = {Key::SelectAll, Key::Copy}});

    EXPECT_EQ(edit.copiedText, "abcdef");
}

TEST(TextAreaSelectionTest, TextArea_CopyReportsTheSelectionAndChangesNothing)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1, .anchor = 4},
        Keyboard{.keys = {Key::Copy}});

    EXPECT_EQ(edit.copiedText, "bcd");
    EXPECT_EQ(edit.text, "abcdef");
    EXPECT_EQ(edit.cursor, 1U);
    EXPECT_EQ(edit.anchor, 4U);
}

TEST(TextAreaSelectionTest, TextArea_CutReportsTheSelectionAndTakesItAway)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1, .anchor = 4},
        Keyboard{.keys = {Key::Cut}});

    EXPECT_EQ(edit.copiedText, "bcd");
    EXPECT_EQ(edit.text, "aef");
    EXPECT_EQ(edit.cursor, 1U);
    EXPECT_EQ(edit.anchor, 1U);
}

TEST(TextAreaSelectionTest, TextArea_CopyingNothingReportsNothingAtAll)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = "abcdef", .cursor = 2},
        Keyboard{.keys = {Key::Copy, Key::Cut}});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaSelectionTest, TextArea_APressPutsTheCaretWhereItLanded)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcd\nefgh", .cursor = 0},
        Keyboard{},
        Pointer{
            .positionPoint = cellAt(1, 2), .down = true, .pressed = true});

    EXPECT_EQ(edit.cursor, 7U);
    EXPECT_EQ(edit.anchor, 7U);
    EXPECT_EQ(edit.text, "abcd\nefgh");
}

TEST(TextAreaSelectionTest, TextArea_APressPastALinesEndIsThatLinesEnd)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "ab\nefgh", .cursor = 7},
        Keyboard{},
        Pointer{
            .positionPoint = cellAt(0, 9), .down = true, .pressed = true});

    EXPECT_EQ(edit.cursor, 2U);
}

TEST(TextAreaSelectionTest, TextArea_APressBelowTheLastLineIsTheEndOfTheText)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "ab\ncd", .cursor = 0},
        Keyboard{},
        Pointer{
            .positionPoint = cellAt(6, 0), .down = true, .pressed = true});

    EXPECT_EQ(edit.cursor, 5U);
}

TEST(TextAreaSelectionTest, TextArea_APressWhereTheCaretAlreadyIsReportsNothing)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = "abcd", .cursor = 2},
        Keyboard{},
        Pointer{
            .positionPoint = cellAt(0, 2), .down = true, .pressed = true});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaSelectionTest, TextArea_AnExtendingPressLeavesTheFarEndWhereItIs)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1},
        Keyboard{},
        Pointer{
            .positionPoint = cellAt(0, 4),
            .down = true,
            .pressed = true,
            .extendsSelection = true});

    EXPECT_EQ(edit.cursor, 4U);
    EXPECT_EQ(edit.anchor, 1U);
}

TEST(TextAreaSelectionTest, TextArea_ADragWithNoPressOfItsOwnStillSelects)
{
    const auto edit = editOf(
        TextAreaSpec{
            .text = "abcdef",
            .cursor = 1,
            .anchor = 1,
            .dragging = antwika::ui::DragOrigin::Text},
        Keyboard{},
        Pointer{
            .positionPoint = cellAt(
                0,
                5), .down = true, .extendsSelection = true});

    EXPECT_EQ(edit.cursor, 5U);
    EXPECT_EQ(edit.anchor, 1U);
}

TEST(TextAreaSelectionTest, TextArea_ABarDragWobblingIntoTheTextSelectsNothing)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = "abcdef",
            .cursor = 1,
            .dragging = antwika::ui::DragOrigin::Track},
        Keyboard{},
        Pointer{
            .positionPoint = cellAt(
                0,
                5), .down = true, .extendsSelection = true});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaSelectionTest, TextArea_APressInTheTextReportsATextHome)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1},
        Keyboard{},
        Pointer{
            .positionPoint = cellAt(0, 3), .down = true, .pressed = true});

    ASSERT_TRUE(frame.interactions.areaPress.has_value());
    EXPECT_EQ(
        frame.interactions.areaPress->homeOrigin,
        antwika::ui::DragOrigin::Text);
}

TEST(TextAreaSelectionTest, TextArea_AHeldButtonThatIsNotDraggingMovesNothing)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1},
        Keyboard{},
        Pointer{.positionPoint = cellAt(0, 5), .down = true});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaSelectionTest, TextArea_APressOutsideTheTextMovesNoCaret)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .widthSizing = antwika::ui::getFixedSize(30),
            .heightSizing = antwika::ui::getFixedSize(16),
            .text = "abcdef",
            .cursor = 1},
        Keyboard{},
        Pointer{
            .positionPoint = Point{.x = 150, .y = 90},
            .down = true,
            .pressed = true});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaSelectionTest, TextArea_APressAmendsWhateverTheKeysCameTo)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcd", .cursor = 4},
        Keyboard{.keys = {Key::Character}, .typedText = "X"},
        Pointer{
            .positionPoint = cellAt(0, 1), .down = true, .pressed = true});

    EXPECT_EQ(edit.text, "abcdX");
    EXPECT_EQ(edit.cursor, 1U);
}

TEST(TextAreaSelectionTest, TextArea_APressOnAnUnfocusedAreaMovesNoCaret)
{
    Context uiContext{
        kCanvasSize,
        getPlainTheme(),
        Pointer{
            .positionPoint = cellAt(0, 2), .down = true, .pressed = true}};

    uiContext.textArea(
        TextAreaSpec{.widgetId = kCodeWidget, .text = "abcd", .cursor = 0});

    EXPECT_FALSE(uiContext.build().interactions.edit.has_value());
}

TEST(TextAreaSelectionTest, TextArea_APressInsideASelectionCollapsesIt)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 2, .anchor = 5},
        Keyboard{},
        Pointer{
            .positionPoint = cellAt(0, 2), .down = true, .pressed = true});

    EXPECT_EQ(edit.cursor, 2U);
    EXPECT_EQ(edit.anchor, 2U);
}

TEST(TextAreaSelectionTest, TextArea_AnUnnamedAreaStillTypesAndNeverScrolls)
{
    Context uiContext{
        kCanvasSize,
        getPlainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Character}, .typedText = "X"}};

    uiContext.textArea(
        TextAreaSpec{.text = "ab", .cursor = 2, .focused = true});

    const auto frame = uiContext.build();

    ASSERT_TRUE(frame.interactions.edit.has_value());
    EXPECT_EQ(frame.interactions.edit->text, "abX");
    EXPECT_FALSE(frame.interactions.scrollChange.has_value());
}

TEST(TextAreaSelectionTest, TextArea_APressAmendsNoOtherWidgetsEdit)
{
    Context uiContext{
        kCanvasSize,
        getPlainTheme(),
        Pointer{
            .positionPoint = cellAt(0, 3), .down = true, .pressed = true},
        Keyboard{.keys = {Key::Character}, .typedText = "X"}};

    uiContext.textArea(TextAreaSpec{
        .widgetId = kCodeWidget,
        .text = "abcdef",
        .cursor = 0,
        .focused = true});

    uiContext.textField(
        {.widgetId = WidgetId{7}, .text = "zz", .cursor = 2, .focused = true});

    const auto frame = uiContext.build();

    ASSERT_TRUE(frame.interactions.edit.has_value());
    EXPECT_EQ(frame.interactions.edit->fieldWidget, kCodeWidget);
    EXPECT_EQ(frame.interactions.edit->text, "abcdef");
    EXPECT_EQ(frame.interactions.edit->cursor, 3U);
}

TEST(TextAreaSelectionTest, TextArea_FinishingTwiceAnswersTheSameWay)
{
    Context uiContext{
        kCanvasSize,
        getPlainTheme(),
        Pointer{
            .positionPoint = cellAt(1, 2), .down = true, .pressed = true}};

    uiContext.textArea(TextAreaSpec{
        .widgetId = kCodeWidget,
        .text = "abcd\nefgh",
        .cursor = 0,
        .focused = true});

    const auto first = uiContext.build();
    const auto second = uiContext.build();

    ASSERT_TRUE(first.interactions.edit.has_value());
    ASSERT_TRUE(second.interactions.edit.has_value());
    EXPECT_EQ(first.interactions.edit->cursor, 7U);
    EXPECT_EQ(second.interactions.edit->cursor, 7U);
    EXPECT_EQ(first.interactions, second.interactions);
}
