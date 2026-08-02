#include <gtest/gtest.h>

#include <cstddef>
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
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::fixedSize;
using antwika::ui::Key;
using antwika::ui::Keyboard;
using antwika::ui::Pointer;
using antwika::ui::TextAreaSpec;
using antwika::ui::Theme;
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kInk{.red = 200, .green = 210, .blue = 220};
    constexpr Color kMuted{.red = 100, .green = 110, .blue = 105};
    constexpr Color kField{.red = 20, .green = 20, .blue = 20};
    constexpr Color kFocused{.red = 30, .green = 30, .blue = 30};
    constexpr Color kCaret{.red = 250, .green = 250, .blue = 250};

    constexpr WidgetId kCode{9};

    constexpr Size kCanvas{.width = 200, .height = 100};

    Theme plainTheme()
    {
        return Theme{
            .text = kInk,
            .muted = kMuted,
            .field = kField,
            .fieldFocused = kFocused,
            .caret = kCaret,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    /**
     * @brief Collect every piece of text in a picture.
     * @param commands The picture to read.
     * @return The strings, in the order they are drawn.
     */
    [[nodiscard]] std::vector<std::string> textsOf(
        const DrawList &commands)
    {
        std::vector<std::string> texts;

        for (const auto &command : commands)
        {
            if (const auto *text = std::get_if<DrawText>(&command))
            {
                texts.push_back(text->text);
            }
        }

        return texts;
    }

    /**
     * @brief Find the caret's bar in a picture.
     * @param commands The picture to read.
     * @return Every fill drawn in the caret's colour.
     */
    [[nodiscard]] std::vector<FillRect> caretsOf(
        const DrawList &commands)
    {
        std::vector<FillRect> carets;

        for (const auto &command : commands)
        {
            const auto *fill = std::get_if<FillRect>(&command);

            if (fill != nullptr && fill->color == kCaret)
            {
                carets.push_back(*fill);
            }
        }

        return carets;
    }

    /**
     * @brief Describe an area holding some text and draw nothing else.
     * @param text What the area holds.
     * @return The picture.
     */
    [[nodiscard]] DrawList pictureOf(std::string_view text)
    {
        Context ui{kCanvas, plainTheme()};

        ui.textArea(TextAreaSpec{.text = text});

        return ui.finish().commands;
    }

    /**
     * @brief Collect every piece of text with where it was drawn.
     * @param commands The picture to read.
     * @return The draws, in the order they are drawn.
     */
    [[nodiscard]] std::vector<DrawText> drawnTextsOf(
        const DrawList &commands)
    {
        std::vector<DrawText> drawn;

        for (const auto &command : commands)
        {
            if (const auto *text = std::get_if<DrawText>(&command))
            {
                drawn.push_back(*text);
            }
        }

        return drawn;
    }

    /**
     * @brief Type into an area and get back what the edit came to.
     * @param text What the area holds.
     * @param cursor Where the caret sits in it.
     * @param keyboard What arrived this frame.
     * @return The frame, whose interactions carry the edit.
     */
    [[nodiscard]] antwika::ui::Frame typeInto(
        std::string_view text,
        std::size_t cursor,
        const Keyboard &keyboard)
    {
        Context ui{kCanvas, plainTheme(), Pointer{}, keyboard};

        ui.textArea(TextAreaSpec{
            .id = kCode,
            .text = text,
            .cursor = cursor,
            .focused = true});

        return ui.finish();
    }
} // namespace

// A line wider than the pane is cut at its edge, never compressed.
// Shrunk, the glyph at one column was a character far along.
// A click there landed the caret somewhere else entirely.
TEST(TextAreaTest, CutsAWideLineAtThePaneInsteadOfShrinkingIt)
{
    // Sixty characters against a 200-pixel pane: 33 whole columns.
    const std::string line(60, 'x');

    Context ui{kCanvas, plainTheme()};

    ui.textArea(TextAreaSpec{
        .id = kCode,
        .width = antwika::ui::kGrow,
        .height = antwika::ui::kGrow,
        .text = line,
        .cursor = 40,
        .focused = true});

    const auto texts = textsOf(ui.finish().commands);

    ASSERT_EQ(texts.size(), 1U);
    EXPECT_EQ(texts[0], std::string(33, 'x'));
}

TEST(TextAreaTest, DrawsOneLinePerLineBreak)
{
    Context ui{kCanvas, plainTheme()};

    ui.textArea(TextAreaSpec{.text = "one\ntwo\nthree"});

    const auto texts = textsOf(ui.finish().commands);

    ASSERT_EQ(3U, texts.size());
    EXPECT_EQ("one", texts.at(0));
    EXPECT_EQ("two", texts.at(1));
    EXPECT_EQ("three", texts.at(2));
}

TEST(TextAreaTest, EachLineIsDrawnBelowTheOneBeforeIt)
{
    const auto drawn = drawnTextsOf(pictureOf("one\ntwo"));

    ASSERT_EQ(2U, drawn.size());
    EXPECT_EQ(drawn.at(0).origin.x, drawn.at(1).origin.x);
    EXPECT_LT(drawn.at(0).origin.y, drawn.at(1).origin.y);
}

TEST(TextAreaTest, ABlankLineStillTakesUpALine)
{
    // An empty text node measures nothing at all.
    // A row holding one alone would collapse.
    const auto apart = drawnTextsOf(pictureOf("one\n\ntwo"));
    const auto together = drawnTextsOf(pictureOf("one\ntwo"));

    ASSERT_EQ(2U, apart.size());
    ASSERT_EQ(2U, together.size());

    const auto spaced = apart.at(1).origin.y - apart.at(0).origin.y;
    const auto tight =
        together.at(1).origin.y - together.at(0).origin.y;

    EXPECT_EQ(2 * tight, spaced);
}

TEST(TextAreaTest, TheCaretSitsOnTheLineTheCursorIsIn)
{
    Context ui{kCanvas, plainTheme()};

    ui.textArea(
        TextAreaSpec{.text = "one\ntwo", .cursor = 5, .focused = true});

    const auto commands = ui.finish().commands;
    const auto carets = caretsOf(commands);

    ASSERT_EQ(1U, carets.size());

    // Split as "t" and "wo", so the second line is two draws.
    const auto texts = textsOf(commands);

    ASSERT_EQ(3U, texts.size());
    EXPECT_EQ("one", texts.at(0));
    EXPECT_EQ("t", texts.at(1));
    EXPECT_EQ("wo", texts.at(2));
}

TEST(TextAreaTest, ACursorAtALineBreakBelongsToTheLineItEnds)
{
    Context ui{kCanvas, plainTheme()};

    ui.textArea(
        TextAreaSpec{.text = "one\ntwo", .cursor = 3, .focused = true});

    const auto commands = ui.finish().commands;

    ASSERT_EQ(1U, caretsOf(commands).size());

    const auto texts = textsOf(commands);

    ASSERT_EQ(2U, texts.size());
    EXPECT_EQ("one", texts.at(0));
    EXPECT_EQ("two", texts.at(1));
}

TEST(TextAreaTest, AnUnfocusedAreaDrawsNoCaretAndReportsNoEdit)
{
    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Character}, .typed = "c"}};

    ui.textArea(TextAreaSpec{.id = kCode, .text = "one"});

    const auto frame = ui.finish();

    EXPECT_TRUE(caretsOf(frame.commands).empty());
    EXPECT_FALSE(frame.interactions.edit.has_value());
    EXPECT_EQ(kField, std::get<FillRect>(frame.commands.at(0)).color);
}

TEST(TextAreaTest, AFocusedAreaTakesTheFocusedFill)
{
    Context ui{kCanvas, plainTheme()};

    ui.textArea(TextAreaSpec{.text = "one", .focused = true});

    const auto commands = ui.finish().commands;

    EXPECT_EQ(kFocused, std::get<FillRect>(commands.at(0)).color);
}

TEST(TextAreaTest, APlaceholderShowsOnlyWhileThereIsNothingToShow)
{
    Context empty{kCanvas, plainTheme()};

    empty.textArea(TextAreaSpec{.placeholder = "write here"});

    const auto shown = textsOf(empty.finish().commands);

    ASSERT_EQ(1U, shown.size());
    EXPECT_EQ("write here", shown.at(0));

    Context held{kCanvas, plainTheme()};

    held.textArea(
        TextAreaSpec{.text = "one", .placeholder = "write here"});

    const auto texts = textsOf(held.finish().commands);

    ASSERT_EQ(1U, texts.size());
    EXPECT_EQ("one", texts.at(0));
}

TEST(TextAreaTest, TypingGoesInWhereTheCaretIs)
{
    const auto frame = typeInto(
        "ab\ncd",
        4,
        Keyboard{.keys = {Key::Character}, .typed = "X"});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(kCode, edit->field);
    EXPECT_EQ("ab\ncXd", edit->text);
    EXPECT_EQ(5U, edit->cursor);
}

TEST(TextAreaTest, EnterWritesALineBreakRatherThanSubmitting)
{
    const auto frame =
        typeInto("ab", 2, Keyboard{.keys = {Key::Activate}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ("ab\n", edit->text);
    EXPECT_EQ(3U, edit->cursor);
    EXPECT_FALSE(edit->submitted);
}

TEST(TextAreaTest, BackspaceTakesALineBreakLikeAnyOtherCharacter)
{
    const auto frame =
        typeInto("ab\ncd", 3, Keyboard{.keys = {Key::Backspace}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ("abcd", edit->text);
    EXPECT_EQ(2U, edit->cursor);
}

TEST(TextAreaTest, MoveUpKeepsTheColumn)
{
    // From "def"'s middle to "abc"'s.
    const auto frame =
        typeInto("abc\ndef", 6, Keyboard{.keys = {Key::MoveUp}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(2U, edit->cursor);
}

TEST(TextAreaTest, MoveUpStopsAtTheEndOfAShorterLine)
{
    const auto frame =
        typeInto("a\nlonger", 7, Keyboard{.keys = {Key::MoveUp}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(1U, edit->cursor);
}

TEST(TextAreaTest, MoveUpFromTheFirstLineDoesNothing)
{
    const auto frame =
        typeInto("abc\ndef", 1, Keyboard{.keys = {Key::MoveUp}});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaTest, MoveDownKeepsTheColumn)
{
    const auto frame =
        typeInto("abc\ndef", 2, Keyboard{.keys = {Key::MoveDown}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(6U, edit->cursor);
}

TEST(TextAreaTest, MoveDownStopsAtTheEndOfAShorterLine)
{
    const auto frame =
        typeInto("longer\na", 5, Keyboard{.keys = {Key::MoveDown}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(8U, edit->cursor);
}

TEST(TextAreaTest, MoveDownFromTheLastLineDoesNothing)
{
    const auto frame =
        typeInto("abc\ndef", 5, Keyboard{.keys = {Key::MoveDown}});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaTest, HomeLandsOnTheCaretsOwnLinesStart)
{
    const auto frame = typeInto(
        "abc\ndef", 6, Keyboard{.keys = {Key::MoveLineStart}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(4U, edit->cursor);
}

TEST(TextAreaTest, EndLandsOnTheCaretsOwnLinesBreak)
{
    const auto frame = typeInto(
        "abc\ndef\nghi", 5, Keyboard{.keys = {Key::MoveLineEnd}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(7U, edit->cursor);
}

TEST(TextAreaTest, HomeAtALinesStartDoesNothing)
{
    const auto frame = typeInto(
        "abc\ndef", 4, Keyboard{.keys = {Key::MoveLineStart}});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaTest, EndOnTheLastLineLandsOnTheTextsEnd)
{
    const auto frame = typeInto(
        "abc\ndef", 5, Keyboard{.keys = {Key::MoveLineEnd}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(7U, edit->cursor);
}

TEST(TextAreaTest, TheHorizontalKeysWalkOverALineBreak)
{
    const auto back =
        typeInto("ab\ncd", 3, Keyboard{.keys = {Key::MoveLeft}});

    ASSERT_TRUE(back.interactions.edit.has_value());
    EXPECT_EQ(2U, back.interactions.edit->cursor);

    const auto on =
        typeInto("ab\ncd", 2, Keyboard{.keys = {Key::MoveRight}});

    ASSERT_TRUE(on.interactions.edit.has_value());
    EXPECT_EQ(3U, on.interactions.edit->cursor);
}

TEST(TextAreaTest, EscapeIsReportedAsACancelledEdit)
{
    const auto frame =
        typeInto("ab", 2, Keyboard{.keys = {Key::Cancel}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_TRUE(edit->cancelled);
}

TEST(TextAreaTest, AQuietFrameReportsNoEdit)
{
    const auto frame = typeInto("ab", 2, Keyboard{});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaTest, ACursorPastTheEndIsTheEnd)
{
    const auto frame = typeInto(
        "ab\ncd",
        99,
        Keyboard{.keys = {Key::Character}, .typed = "X"});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ("ab\ncdX", edit->text);
}

TEST(TextAreaTest, APressOnTheBoxNamesTheArea)
{
    const Pointer pointer{
        .position = Point{.x = 4, .y = 2}, .pressed = true};

    Context ui{kCanvas, plainTheme(), pointer};

    ui.textArea(TextAreaSpec{.id = kCode, .text = "one"});

    EXPECT_EQ(kCode, ui.finish().interactions.activated);
}

TEST(TextAreaTest, TabReachesAnAreaAndTheNextFrameTypesIntoIt)
{
    Context first{
        kCanvas,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext}}};

    first.textArea(TextAreaSpec{.id = kCode, .text = "ab"});

    const auto reached = first.finish();

    EXPECT_EQ(kCode, reached.interactions.focused);

    Context second{
        kCanvas,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Character}, .typed = "c"},
        reached.interactions.focused};

    second.textArea(TextAreaSpec{.id = kCode, .text = "ab"});

    const auto frame = second.finish();

    ASSERT_TRUE(frame.interactions.edit.has_value());
    EXPECT_EQ("abc", frame.interactions.edit->text);
    EXPECT_EQ(1U, caretsOf(frame.commands).size());
}

TEST(TextAreaTest, AFixedSizeAreaTakesExactlyThatMuch)
{
    Context ui{kCanvas, plainTheme()};

    ui.textArea(TextAreaSpec{
        .width = fixedSize(80), .height = fixedSize(40), .text = "ab"});

    const auto commands = ui.finish().commands;
    const auto box = std::get<FillRect>(commands.at(0)).rect;

    EXPECT_EQ(80U, box.size.width);
    EXPECT_EQ(40U, box.size.height);
}

TEST(TextAreaTest, ACaretIsAtLeastOnePixelWideAtAZeroScale)
{
    auto theme = plainTheme();
    theme.textScale = 0;

    Context ui{kCanvas, theme};

    ui.textArea(TextAreaSpec{.text = "ab", .focused = true});

    const auto carets = caretsOf(ui.finish().commands);

    ASSERT_EQ(1U, carets.size());
    EXPECT_EQ(1U, carets.at(0).rect.size.width);
}

TEST(TextAreaTest, AnEmptyAreaWithNoPlaceholderDrawsNoTextAtAll)
{
    Context ui{kCanvas, plainTheme()};

    ui.textArea(TextAreaSpec{});

    EXPECT_TRUE(textsOf(ui.finish().commands).empty());
}

TEST(TextAreaTest, MoveUpWithTheCaretAtTheVeryStartDoesNothing)
{
    const auto frame =
        typeInto("abc\ndef", 0, Keyboard{.keys = {Key::MoveUp}});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}
