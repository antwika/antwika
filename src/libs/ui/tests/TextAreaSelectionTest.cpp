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
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kInk{.red = 200, .green = 210, .blue = 220};
    constexpr Color kPicked{.red = 40, .green = 70, .blue = 120};

    constexpr WidgetId kCode{9};

    constexpr Size kCanvas{.width = 200, .height = 100};

    // One glyph cell at this scale.
    // Which is what a click is measured in, and what a ground covers.
    constexpr std::int32_t kAdvance = 6;
    constexpr std::int32_t kLineHeight = 8;

    Theme plainTheme()
    {
        return Theme{
            .text = kInk,
            .selection = kPicked,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    /**
     * @brief Describe one focused area over a whole frame's input.
     * @param spec What the area is being asked for; its id and focus
     * are filled in here.
     * @param keyboard What arrived this frame.
     * @param pointer What the pointer is doing.
     * @return The finished frame.
     */
    [[nodiscard]] Frame frameOf(
        TextAreaSpec spec,
        const Keyboard &keyboard = {},
        const Pointer &pointer = {})
    {
        spec.id = kCode;
        spec.focused = true;

        Context ui{kCanvas, plainTheme(), pointer, keyboard};

        ui.textArea(spec);

        return ui.finish();
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

    /**
     * @brief Collect every piece of text drawn on the selection's own
     * ground.
     *
     * A piece is either wholly selected or not at all, which is what
     * makes this the whole selection rather than a sample of it.
     *
     * @param commands The picture to read.
     * @return The selected strings, in drawing order.
     */
    [[nodiscard]] std::vector<std::string> pickedOf(
        const DrawList &commands)
    {
        std::vector<std::string> picked;
        std::optional<FillRect> ground;

        for (const auto &command : commands)
        {
            if (const auto *fill = std::get_if<FillRect>(&command))
            {
                ground = fill->color == kPicked
                             ? std::optional{*fill}
                             : std::nullopt;

                continue;
            }

            const auto *text = std::get_if<DrawText>(&command);

            if (text != nullptr && ground.has_value()
                && ground->rect.origin == text->origin)
            {
                picked.push_back(text->text);
            }

            ground.reset();
        }

        return picked;
    }

    [[nodiscard]] std::size_t groundsOf(const DrawList &commands)
    {
        std::size_t grounds = 0;

        for (const auto &command : commands)
        {
            const auto *fill = std::get_if<FillRect>(&command);

            if (fill != nullptr && fill->color == kPicked)
            {
                ++grounds;
            }
        }

        return grounds;
    }

    // Somewhere inside one cell of the pane.
    // It sits at the origin, since this theme pads nothing.
    [[nodiscard]] Point cellAt(std::int32_t line, std::int32_t column)
    {
        return Point{
            .x = column * kAdvance + 1, .y = line * kLineHeight + 1};
    }
} // namespace

TEST(TextAreaSelectionTest, AnAreaToldNothingHasNothingSelected)
{
    const auto picture =
        frameOf(TextAreaSpec{.text = "abcd", .cursor = 2}).commands;

    EXPECT_EQ(groundsOf(picture), 0U);
}

TEST(TextAreaSelectionTest, DrawsTheSelectedCharactersOnTheirOwnGround)
{
    const auto picture =
        frameOf(TextAreaSpec{.text = "abcdef", .cursor = 4, .anchor = 1})
            .commands;

    EXPECT_EQ(pickedOf(picture), std::vector<std::string>{"bcd"});
}

// Which end is the caret changes nothing about what is selected.
TEST(TextAreaSelectionTest, TheEndsAreInterchangeable)
{
    const auto forwards =
        frameOf(TextAreaSpec{.text = "abcdef", .cursor = 4, .anchor = 1});

    const auto backwards =
        frameOf(TextAreaSpec{.text = "abcdef", .cursor = 1, .anchor = 4});

    EXPECT_EQ(
        pickedOf(forwards.commands), pickedOf(backwards.commands));
}

// A selection spanning a break shows on every line it crosses.
// And on the break itself, or a blank line would read as a hole.
TEST(TextAreaSelectionTest, ASelectionAcrossLinesIsDrawnOnEachOfThem)
{
    const auto picture =
        frameOf(TextAreaSpec{
                    .text = "ab\n\ncd", .cursor = 5, .anchor = 1})
            .commands;

    const std::vector<std::string> expected{"b", "c"};

    EXPECT_EQ(pickedOf(picture), expected);

    // Two runs of text, and a cell of ground on each of the two breaks.
    EXPECT_EQ(groundsOf(picture), 4U);
}

TEST(TextAreaSelectionTest, AnUnfocusedAreaShowsNoSelection)
{
    Context ui{kCanvas, plainTheme()};

    ui.textArea(
        TextAreaSpec{.text = "abcdef", .cursor = 4, .anchor = 1});

    EXPECT_EQ(groundsOf(ui.finish().commands), 0U);
}

TEST(TextAreaSelectionTest, ShiftAndAnArrowLeavesTheFarEndWhereItIs)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcd", .cursor = 1},
        Keyboard{.keys = {Key::SelectRight, Key::SelectRight}});

    EXPECT_EQ(edit.cursor, 3U);
    EXPECT_EQ(edit.anchor, 1U);
}

TEST(TextAreaSelectionTest, ShiftAndTheLeftArrowSelectsBackwards)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcd", .cursor = 3},
        Keyboard{.keys = {Key::SelectLeft, Key::SelectLeft}});

    EXPECT_EQ(edit.cursor, 1U);
    EXPECT_EQ(edit.anchor, 3U);
}

TEST(TextAreaSelectionTest, SelectingUpAndDownWalksTheLines)
{
    const auto down = editOf(
        TextAreaSpec{.text = "abcd\nefgh", .cursor = 1},
        Keyboard{.keys = {Key::SelectDown}});

    EXPECT_EQ(down.cursor, 6U);
    EXPECT_EQ(down.anchor, 1U);

    const auto up = editOf(
        TextAreaSpec{.text = "abcd\nefgh", .cursor = 6},
        Keyboard{.keys = {Key::SelectUp}});

    EXPECT_EQ(up.cursor, 1U);
    EXPECT_EQ(up.anchor, 6U);
}

// Nothing to select towards is nothing selected, and nothing reported.
TEST(TextAreaSelectionTest, SelectingPastEitherEndDoesNothing)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = "ab", .cursor = 0},
        Keyboard{.keys = {Key::SelectLeft, Key::SelectUp}});

    EXPECT_FALSE(frame.interactions.edit.has_value());

    const auto other = frameOf(
        TextAreaSpec{.text = "ab", .cursor = 2},
        Keyboard{.keys = {Key::SelectRight, Key::SelectDown}});

    EXPECT_FALSE(other.interactions.edit.has_value());
}

// The one thing every editor agrees about.
// A plain move off a selection lands on the end it heads for.
TEST(TextAreaSelectionTest, APlainMoveCollapsesToTheEndItIsHeadingFor)
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

TEST(TextAreaSelectionTest, MovingUpOrDownCollapsesTheSelectionToo)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcd\nefgh", .cursor = 7, .anchor = 9},
        Keyboard{.keys = {Key::MoveUp}});

    EXPECT_EQ(edit.cursor, 2U);
    EXPECT_EQ(edit.anchor, 2U);
}

TEST(TextAreaSelectionTest, HomeAndEndCollapseTheSelectionToo)
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

TEST(TextAreaSelectionTest, ShiftedHomeAndEndSelectToTheLinesEnds)
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

TEST(TextAreaSelectionTest, TypingOverASelectionTakesTheWholeOfIt)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1, .anchor = 4},
        Keyboard{.keys = {Key::Character}, .typed = "X"});

    EXPECT_EQ(edit.text, "aXef");
    EXPECT_EQ(edit.cursor, 2U);
    EXPECT_EQ(edit.anchor, 2U);
}

TEST(TextAreaSelectionTest, EnterOverASelectionReplacesItWithABreak)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1, .anchor = 4},
        Keyboard{.keys = {Key::Activate}});

    EXPECT_EQ(edit.text, "a\nef");
}

TEST(TextAreaSelectionTest, BackspaceAndDeleteBothTakeAWholeSelection)
{
    const auto back = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1, .anchor = 4},
        Keyboard{.keys = {Key::Backspace}});

    EXPECT_EQ(back.text, "aef");
    EXPECT_EQ(back.cursor, 1U);

    const auto forward = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 4, .anchor = 1},
        Keyboard{.keys = {Key::Delete}});

    EXPECT_EQ(forward.text, "aef");
    EXPECT_EQ(forward.cursor, 1U);
}

TEST(TextAreaSelectionTest, DeleteTakesTheCharacterTheCaretSitsBefore)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abc", .cursor = 1},
        Keyboard{.keys = {Key::Delete}});

    EXPECT_EQ(edit.text, "ac");
    EXPECT_EQ(edit.cursor, 1U);
}

TEST(TextAreaSelectionTest, DeleteAtTheEndHasNothingToTake)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = "abc", .cursor = 3},
        Keyboard{.keys = {Key::Delete}});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

// What holds them afterwards is the caller's.
// Which is why this is reported rather than kept.
TEST(TextAreaSelectionTest, CopyReportsTheSelectionAndChangesNothing)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1, .anchor = 4},
        Keyboard{.keys = {Key::Copy}});

    EXPECT_EQ(edit.copied, "bcd");
    EXPECT_EQ(edit.text, "abcdef");
    EXPECT_EQ(edit.cursor, 1U);
    EXPECT_EQ(edit.anchor, 4U);
}

TEST(TextAreaSelectionTest, CutReportsTheSelectionAndTakesItAway)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1, .anchor = 4},
        Keyboard{.keys = {Key::Cut}});

    EXPECT_EQ(edit.copied, "bcd");
    EXPECT_EQ(edit.text, "aef");
    EXPECT_EQ(edit.cursor, 1U);
    EXPECT_EQ(edit.anchor, 1U);
}

TEST(TextAreaSelectionTest, CopyingNothingReportsNothingAtAll)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = "abcdef", .cursor = 2},
        Keyboard{.keys = {Key::Copy, Key::Cut}});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

// A press is measured against the cells the pane was drawn in.
// So which character it landed on is arithmetic.
TEST(TextAreaSelectionTest, APressPutsTheCaretWhereItLanded)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcd\nefgh", .cursor = 0},
        Keyboard{},
        Pointer{
            .position = cellAt(1, 2), .down = true, .pressed = true});

    EXPECT_EQ(edit.cursor, 7U);
    EXPECT_EQ(edit.anchor, 7U);
    EXPECT_EQ(edit.text, "abcd\nefgh");
}

TEST(TextAreaSelectionTest, APressPastALinesEndIsThatLinesEnd)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "ab\nefgh", .cursor = 7},
        Keyboard{},
        Pointer{
            .position = cellAt(0, 9), .down = true, .pressed = true});

    EXPECT_EQ(edit.cursor, 2U);
}

TEST(TextAreaSelectionTest, APressBelowTheLastLineIsTheEndOfTheText)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "ab\ncd", .cursor = 0},
        Keyboard{},
        Pointer{
            .position = cellAt(6, 0), .down = true, .pressed = true});

    EXPECT_EQ(edit.cursor, 5U);
}

// A press that changed nothing is a frame that changed nothing.
TEST(TextAreaSelectionTest, APressWhereTheCaretAlreadyIsReportsNothing)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = "abcd", .cursor = 2},
        Keyboard{},
        Pointer{
            .position = cellAt(0, 2), .down = true, .pressed = true});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

// What shift-clicking means, and what dragging one out means.
TEST(TextAreaSelectionTest, AnExtendingPressLeavesTheFarEndWhereItIs)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1},
        Keyboard{},
        Pointer{
            .position = cellAt(0, 4),
            .down = true,
            .pressed = true,
            .extends = true});

    EXPECT_EQ(edit.cursor, 4U);
    EXPECT_EQ(edit.anchor, 1U);
}

// Read while a button is down as well as on the press itself.
// So a caller reporting a drag gets a selection following the pointer.
TEST(TextAreaSelectionTest, ADragWithNoPressOfItsOwnStillSelects)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1, .anchor = 1},
        Keyboard{},
        Pointer{
            .position = cellAt(0, 5), .down = true, .extends = true});

    EXPECT_EQ(edit.cursor, 5U);
    EXPECT_EQ(edit.anchor, 1U);
}

TEST(TextAreaSelectionTest, AHeldButtonThatIsNotDraggingMovesNothing)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = "abcdef", .cursor = 1},
        Keyboard{},
        Pointer{.position = cellAt(0, 5), .down = true});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaSelectionTest, APressOutsideTheTextMovesNoCaret)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .width = antwika::ui::fixedSize(30),
            .height = antwika::ui::fixedSize(16),
            .text = "abcdef",
            .cursor = 1},
        Keyboard{},
        Pointer{
            .position = Point{.x = 150, .y = 90},
            .down = true,
            .pressed = true});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

// One frame that both typed and clicked reports one edit.
// The click is measured against whatever the typing came to.
TEST(TextAreaSelectionTest, APressAmendsWhateverTheKeysCameTo)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcd", .cursor = 4},
        Keyboard{.keys = {Key::Character}, .typed = "X"},
        Pointer{
            .position = cellAt(0, 1), .down = true, .pressed = true});

    EXPECT_EQ(edit.text, "abcdX");
    EXPECT_EQ(edit.cursor, 1U);
}

// The caret is drawn only for a focused area.
// Only a focused area answers a press with one, for the same reason.
TEST(TextAreaSelectionTest, APressOnAnUnfocusedAreaMovesNoCaret)
{
    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{
            .position = cellAt(0, 2), .down = true, .pressed = true}};

    ui.textArea(TextAreaSpec{.id = kCode, .text = "abcd", .cursor = 0});

    EXPECT_FALSE(ui.finish().interactions.edit.has_value());
}

// A press in the middle of a selection takes the selection away.
// Even though the caret itself has not moved.
TEST(TextAreaSelectionTest, APressInsideASelectionCollapsesIt)
{
    const auto edit = editOf(
        TextAreaSpec{.text = "abcdef", .cursor = 2, .anchor = 5},
        Keyboard{},
        Pointer{
            .position = cellAt(0, 2), .down = true, .pressed = true});

    EXPECT_EQ(edit.cursor, 2U);
    EXPECT_EQ(edit.anchor, 2U);
}

// An area nothing can name reports an edit like any other.
// Its caret is the one that is never followed.
// The report would name no area, so nothing could store it.
TEST(TextAreaSelectionTest, AnUnnamedAreaStillTypesAndNeverScrolls)
{
    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Character}, .typed = "X"}};

    ui.textArea(
        TextAreaSpec{.text = "ab", .cursor = 2, .focused = true});

    const auto frame = ui.finish();

    ASSERT_TRUE(frame.interactions.edit.has_value());
    EXPECT_EQ(frame.interactions.edit->text, "abX");
    EXPECT_FALSE(frame.interactions.scrolled.has_value());
}

// Two focused widgets is an ambiguous frame.
// The click still lands on the area, against the area's own text.
TEST(TextAreaSelectionTest, APressAmendsNoOtherWidgetsEdit)
{
    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{
            .position = cellAt(0, 3), .down = true, .pressed = true},
        Keyboard{.keys = {Key::Character}, .typed = "X"}};

    ui.textArea(TextAreaSpec{
        .id = kCode, .text = "abcdef", .cursor = 0, .focused = true});

    ui.textField(
        {.id = WidgetId{7}, .text = "zz", .cursor = 2, .focused = true});

    const auto frame = ui.finish();

    ASSERT_TRUE(frame.interactions.edit.has_value());
    EXPECT_EQ(frame.interactions.edit->field, kCode);
    EXPECT_EQ(frame.interactions.edit->text, "abcdef");
    EXPECT_EQ(frame.interactions.edit->cursor, 3U);
}

// Asking twice gives the same answer.
// A click resolved against the layout has to keep that true.
TEST(TextAreaSelectionTest, FinishingTwiceAnswersTheSameWay)
{
    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{
            .position = cellAt(1, 2), .down = true, .pressed = true}};

    ui.textArea(TextAreaSpec{
        .id = kCode,
        .text = "abcd\nefgh",
        .cursor = 0,
        .focused = true});

    const auto first = ui.finish();
    const auto second = ui.finish();

    EXPECT_EQ(first.interactions, second.interactions);
}
