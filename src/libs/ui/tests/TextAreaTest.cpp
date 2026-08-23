#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/support/DrawListQueries.hpp>

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
using antwika::ui::support::fillsColored;
using antwika::ui::support::textsOf;
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
using antwika::widget::WidgetId;

namespace
{
    constexpr Color kInkColor{.red = 200, .green = 210, .blue = 220};
    constexpr Color kMutedColor{.red = 100, .green = 110, .blue = 105};
    constexpr Color kFieldColor{.red = 20, .green = 20, .blue = 20};
    constexpr Color kFocusedColor{.red = 30, .green = 30, .blue = 30};
    constexpr Color kCaretColor{.red = 250, .green = 250, .blue = 250};

    constexpr WidgetId kCodeWidget{9};

    constexpr Size kCanvasSize{.width = 200, .height = 100};

    Theme plainTheme()
    {
        return Theme{
            .textColor = kInkColor,
            .mutedColor = kMutedColor,
            .fieldColor = kFieldColor,
            .fieldFocusedColor = kFocusedColor,
            .caretColor = kCaretColor,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    [[nodiscard]] DrawList pictureOf(std::string_view text)
    {
        Context uiContext{kCanvasSize, plainTheme()};

        uiContext.textArea(TextAreaSpec{.text = text});

        return uiContext.build().drawList;
    }

    [[nodiscard]] std::vector<DrawText> drawnTextsOf(
        const DrawList &drawList)
    {
        std::vector<DrawText> drawnTexts;

        for (const auto &command : drawList)
        {
            if (const auto *text = std::get_if<DrawText>(&command))
            {
                drawnTexts.push_back(*text);
            }
        }

        return drawnTexts;
    }

    [[nodiscard]] antwika::ui::Frame typeInto(
        std::string_view text,
        std::size_t cursor,
        const Keyboard &keyboard)
    {
        Context uiContext{kCanvasSize, plainTheme(), Pointer{}, keyboard};

        uiContext.textArea(TextAreaSpec{
            .widgetId = kCodeWidget,
            .text = text,
            .cursor = cursor,
            .focused = true});

        return uiContext.build();
    }
}

TEST(TextAreaTest, TextArea_CutsAWideLineAtThePaneInsteadOfShrinkingIt)
{
    const std::string line(60, 'x');

    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textArea(TextAreaSpec{
        .widgetId = kCodeWidget,
        .widthSizing = antwika::ui::kGrowSizing,
        .heightSizing = antwika::ui::kGrowSizing,
        .text = line,
        .cursor = 40,
        .focused = true});

    const auto texts = textsOf(uiContext.build().drawList);

    ASSERT_EQ(texts.size(), 1U);
    EXPECT_EQ(texts[0], std::string(33, 'x'));
}

TEST(TextAreaTest, TextArea_DrawsOneLinePerLineBreak)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textArea(TextAreaSpec{.text = "one\ntwo\nthree"});

    const auto texts = textsOf(uiContext.build().drawList);

    ASSERT_EQ(3U, texts.size());
    EXPECT_EQ("one", texts.at(0));
    EXPECT_EQ("two", texts.at(1));
    EXPECT_EQ("three", texts.at(2));
}

TEST(TextAreaTest, TextArea_EachLineIsDrawnBelowTheOneBeforeIt)
{
    const auto drawnTexts = drawnTextsOf(pictureOf("one\ntwo"));

    ASSERT_EQ(2U, drawnTexts.size());
    EXPECT_EQ(drawnTexts.at(0).originPoint.x, drawnTexts.at(1).originPoint.x);
    EXPECT_LT(drawnTexts.at(0).originPoint.y, drawnTexts.at(1).originPoint.y);
}

TEST(TextAreaTest, TextArea_ABlankLineStillTakesUpALine)
{
    const auto apart = drawnTextsOf(pictureOf("one\n\ntwo"));
    const auto together = drawnTextsOf(pictureOf("one\ntwo"));

    ASSERT_EQ(2U, apart.size());
    ASSERT_EQ(2U, together.size());

    const auto lineSpacing =
        apart.at(1).originPoint.y - apart.at(0).originPoint.y;
    const auto tight =
        together.at(1).originPoint.y - together.at(0).originPoint.y;

    EXPECT_EQ(2 * tight, lineSpacing);
}

TEST(TextAreaTest, TextArea_TheCaretSitsOnTheLineTheCursorIsIn)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textArea(
        TextAreaSpec{.text = "one\ntwo", .cursor = 5, .focused = true});

    const auto commands = uiContext.build().drawList;
    const auto carets = fillsColored(commands, kCaretColor);

    ASSERT_EQ(1U, carets.size());

    const auto texts = textsOf(commands);

    ASSERT_EQ(3U, texts.size());
    EXPECT_EQ("one", texts.at(0));
    EXPECT_EQ("t", texts.at(1));
    EXPECT_EQ("wo", texts.at(2));
}

TEST(TextAreaTest, TextArea_ACursorAtALineBreakBelongsToTheLineItEnds)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textArea(
        TextAreaSpec{.text = "one\ntwo", .cursor = 3, .focused = true});

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(1U, fillsColored(commands, kCaretColor).size());

    const auto texts = textsOf(commands);

    ASSERT_EQ(2U, texts.size());
    EXPECT_EQ("one", texts.at(0));
    EXPECT_EQ("two", texts.at(1));
}

TEST(TextAreaTest, TextArea_AnUnfocusedAreaDrawsNoCaretAndReportsNoEdit)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Character}, .typedText = "c"}};

    uiContext.textArea(TextAreaSpec{.widgetId = kCodeWidget, .text = "one"});

    const auto frame = uiContext.build();

    EXPECT_TRUE(fillsColored(frame.drawList, kCaretColor).empty());
    EXPECT_FALSE(frame.interactions.edit.has_value());
    EXPECT_EQ(kFieldColor, std::get<FillRect>(frame.drawList.at(0)).color);
}

TEST(TextAreaTest, TextArea_AFocusedAreaTakesTheFocusedFill)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textArea(TextAreaSpec{.text = "one", .focused = true});

    const auto commands = uiContext.build().drawList;

    EXPECT_EQ(kFocusedColor, std::get<FillRect>(commands.at(0)).color);
}

TEST(TextAreaTest, TextArea_APlaceholderShowsOnlyWhileThereIsNothingToShow)
{
    Context emptyContext{kCanvasSize, plainTheme()};

    emptyContext.textArea(TextAreaSpec{.placeholder = "write here"});

    const auto shownTexts = textsOf(emptyContext.build().drawList);

    ASSERT_EQ(1U, shownTexts.size());
    EXPECT_EQ("write here", shownTexts.at(0));

    Context context{kCanvasSize, plainTheme()};

    context.textArea(
        TextAreaSpec{.text = "one", .placeholder = "write here"});

    const auto texts = textsOf(context.build().drawList);

    ASSERT_EQ(1U, texts.size());
    EXPECT_EQ("one", texts.at(0));
}

TEST(TextAreaTest, TextArea_TypingGoesInWhereTheCaretIs)
{
    const auto frame = typeInto(
        "ab\ncd",
        4,
        Keyboard{.keys = {Key::Character}, .typedText = "X"});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(kCodeWidget, edit->fieldWidget);
    EXPECT_EQ("ab\ncXd", edit->text);
    EXPECT_EQ(5U, edit->cursor);
}

TEST(TextAreaTest, TextArea_EnterWritesALineBreakRatherThanSubmitting)
{
    const auto frame =
        typeInto("ab", 2, Keyboard{.keys = {Key::Activate}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ("ab\n", edit->text);
    EXPECT_EQ(3U, edit->cursor);
    EXPECT_FALSE(edit->submitted);
}

TEST(TextAreaTest, TextArea_BackspaceTakesALineBreakLikeAnyOtherCharacter)
{
    const auto frame =
        typeInto("ab\ncd", 3, Keyboard{.keys = {Key::Backspace}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ("abcd", edit->text);
    EXPECT_EQ(2U, edit->cursor);
}

TEST(TextAreaTest, TextArea_MoveUpKeepsTheColumn)
{
    const auto frame =
        typeInto("abc\ndef", 6, Keyboard{.keys = {Key::MoveUp}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(2U, edit->cursor);
}

TEST(TextAreaTest, TextArea_MoveUpStopsAtTheEndOfAShorterLine)
{
    const auto frame =
        typeInto("a\nlonger", 7, Keyboard{.keys = {Key::MoveUp}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(1U, edit->cursor);
}

TEST(TextAreaTest, TextArea_MoveUpFromTheFirstLineDoesNothing)
{
    const auto frame =
        typeInto("abc\ndef", 1, Keyboard{.keys = {Key::MoveUp}});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaTest, TextArea_MoveDownKeepsTheColumn)
{
    const auto frame =
        typeInto("abc\ndef", 2, Keyboard{.keys = {Key::MoveDown}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(6U, edit->cursor);
}

TEST(TextAreaTest, TextArea_MoveDownStopsAtTheEndOfAShorterLine)
{
    const auto frame =
        typeInto("longer\na", 5, Keyboard{.keys = {Key::MoveDown}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(8U, edit->cursor);
}

TEST(TextAreaTest, TextArea_MoveDownFromTheLastLineDoesNothing)
{
    const auto frame =
        typeInto("abc\ndef", 5, Keyboard{.keys = {Key::MoveDown}});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaTest, TextArea_HomeLandsOnTheCaretsOwnLinesStart)
{
    const auto frame = typeInto(
        "abc\ndef", 6, Keyboard{.keys = {Key::MoveLineStart}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(4U, edit->cursor);
}

TEST(TextAreaTest, TextArea_EndLandsOnTheCaretsOwnLinesBreak)
{
    const auto frame = typeInto(
        "abc\ndef\nghi", 5, Keyboard{.keys = {Key::MoveLineEnd}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(7U, edit->cursor);
}

TEST(TextAreaTest, TextArea_HomeAtALinesStartDoesNothing)
{
    const auto frame = typeInto(
        "abc\ndef", 4, Keyboard{.keys = {Key::MoveLineStart}});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaTest, TextArea_HomeAtTheVeryStartOfTheTextDoesNothing)
{
    const auto frame = typeInto(
        "abc\ndef", 0, Keyboard{.keys = {Key::MoveLineStart}});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaTest, TextArea_EndOnTheLastLineLandsOnTheTextsEnd)
{
    const auto frame = typeInto(
        "abc\ndef", 5, Keyboard{.keys = {Key::MoveLineEnd}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(7U, edit->cursor);
}

TEST(TextAreaTest, TextArea_TheHorizontalKeysWalkOverALineBreak)
{
    const auto movedLeft =
        typeInto("ab\ncd", 3, Keyboard{.keys = {Key::MoveLeft}});

    ASSERT_TRUE(movedLeft.interactions.edit.has_value());
    EXPECT_EQ(2U, movedLeft.interactions.edit->cursor);

    const auto movedRight =
        typeInto("ab\ncd", 2, Keyboard{.keys = {Key::MoveRight}});

    ASSERT_TRUE(movedRight.interactions.edit.has_value());
    EXPECT_EQ(3U, movedRight.interactions.edit->cursor);
}

TEST(TextAreaTest, TextArea_EscapeIsReportedAsACancelledEdit)
{
    const auto frame =
        typeInto("ab", 2, Keyboard{.keys = {Key::Cancel}});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_TRUE(edit->cancelled);
}

TEST(TextAreaTest, TextArea_AQuietFrameReportsNoEdit)
{
    const auto frame = typeInto("ab", 2, Keyboard{});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaTest, TextArea_ACursorPastTheEndIsTheEnd)
{
    const auto frame = typeInto(
        "ab\ncd",
        99,
        Keyboard{.keys = {Key::Character}, .typedText = "X"});

    const auto edit = frame.interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ("ab\ncdX", edit->text);
}

TEST(TextAreaTest, TextArea_APressOnTheBoxNamesTheArea)
{
    const Pointer pointer{
        .positionPoint = Point{.x = 4, .y = 2}, .pressed = true};

    Context uiContext{kCanvasSize, plainTheme(), pointer};

    uiContext.textArea(TextAreaSpec{.widgetId = kCodeWidget, .text = "one"});

    EXPECT_EQ(kCodeWidget, uiContext.build().interactions.activatedWidget);
}

TEST(TextAreaTest, TextArea_TabReachesAnAreaAndTheNextFrameTypesIntoIt)
{
    Context firstContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext}}};

    firstContext.textArea(TextAreaSpec{.widgetId = kCodeWidget, .text = "ab"});

    const auto firstFrame = firstContext.build();

    EXPECT_EQ(kCodeWidget, firstFrame.interactions.focusedWidget);

    Context secondContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Character}, .typedText = "c"},
        firstFrame.interactions.focusedWidget};

    secondContext.textArea(TextAreaSpec{.widgetId = kCodeWidget, .text = "ab"});

    const auto frame = secondContext.build();

    ASSERT_TRUE(frame.interactions.edit.has_value());
    EXPECT_EQ("abc", frame.interactions.edit->text);
    EXPECT_EQ(1U, fillsColored(frame.drawList, kCaretColor).size());
}

TEST(TextAreaTest, TextArea_AFixedSizeAreaTakesExactlyThatMuch)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textArea(TextAreaSpec{
        .widthSizing = fixedSize(
            80), .heightSizing = fixedSize(40), .text = "ab"});

    const auto commands = uiContext.build().drawList;
    const auto box = std::get<FillRect>(commands.at(0)).rect;

    EXPECT_EQ(80U, box.size.width);
    EXPECT_EQ(40U, box.size.height);
}

TEST(TextAreaTest, TextArea_ACaretIsAtLeastOnePixelWideAtAZeroScale)
{
    auto theme = plainTheme();
    theme.textScale = 0;

    Context uiContext{kCanvasSize, theme};

    uiContext.textArea(TextAreaSpec{.text = "ab", .focused = true});

    const auto carets = fillsColored(uiContext.build().drawList, kCaretColor);

    ASSERT_EQ(1U, carets.size());
    EXPECT_EQ(1U, carets.at(0).rect.size.width);
}

TEST(TextAreaTest, TextArea_AnEmptyAreaWithNoPlaceholderDrawsNoTextAtAll)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textArea(TextAreaSpec{});

    EXPECT_TRUE(textsOf(uiContext.build().drawList).empty());
}

TEST(TextAreaTest, TextArea_MoveUpWithTheCaretAtTheVeryStartDoesNothing)
{
    const auto frame =
        typeInto("abc\ndef", 0, Keyboard{.keys = {Key::MoveUp}});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaTest, TextArea_TheCaretShiftsNoCharacterOfItsLine)
{
    Context stillContext{kCanvasSize, plainTheme()};

    stillContext.textArea(TextAreaSpec{.text = "one\ntwo", .cursor = 0});

    Context carryingContext{kCanvasSize, plainTheme()};

    carryingContext.textArea(
        TextAreaSpec{.text = "one\ntwo", .cursor = 1, .focused = true});

    const auto bareFrame = stillContext.build();
    const auto carryingFrame = carryingContext.build();

    std::vector<DrawText> quietTexts;
    std::vector<DrawText> focusedTexts;

    for (const auto &command : bareFrame.drawList)
    {
        if (const auto *text = std::get_if<DrawText>(&command))
        {
            quietTexts.push_back(*text);
        }
    }

    for (const auto &command : carryingFrame.drawList)
    {
        if (const auto *text = std::get_if<DrawText>(&command))
        {
            focusedTexts.push_back(*text);
        }
    }

    ASSERT_EQ(quietTexts.size(), 2U);
    ASSERT_EQ(focusedTexts.size(), 3U);
    EXPECT_EQ(focusedTexts.at(0).originPoint, quietTexts.at(0).originPoint);
    EXPECT_EQ(
        focusedTexts.at(1).originPoint.x,
        quietTexts.at(0).originPoint.x
            + static_cast<std::int32_t>(
                antwika::gfx::kGlyphAdvance
                * plainTheme().textScale));
    EXPECT_EQ(focusedTexts.at(2).originPoint, quietTexts.at(1).originPoint);
}

TEST(TextAreaTest, TextArea_TheCaretIsDrawnItsThemeWidthAllTheSame)
{
    auto theme = plainTheme();
    theme.textScale = 3;

    Context uiContext{kCanvasSize, theme};

    uiContext.textArea(
        TextAreaSpec{.text = "ab", .cursor = 1, .focused = true});

    const auto carets = fillsColored(uiContext.build().drawList, kCaretColor);

    ASSERT_EQ(1U, carets.size());
    EXPECT_EQ(carets.at(0).rect.size.width, 3U);
}

TEST(TextAreaTest, TextArea_ACaretAtTheClippedEdgeIsCutToNothing)
{
    Context uiContext{
        Size{.width = 20, .height = 100}, plainTheme()};

    uiContext.textArea(TextAreaSpec{
        .text = "a long line of characters",
        .focused = true});

    const auto carets = fillsColored(uiContext.build().drawList, kCaretColor);

    ASSERT_EQ(1U, carets.size());
    EXPECT_EQ(carets.at(0).rect.size.width, 0U);
}
