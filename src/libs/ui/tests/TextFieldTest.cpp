#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/support/DrawListQueries.hpp>

#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/Keyboard.hpp"
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
using antwika::ui::kCaretAtEnd;
using antwika::ui::Pointer;
using antwika::ui::TextFieldSpec;
using antwika::ui::Key;
using antwika::ui::Keyboard;
using antwika::ui::Theme;
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kInkColor{.red = 200, .green = 210, .blue = 220};
    constexpr Color kMutedColor{.red = 100, .green = 110, .blue = 105};
    constexpr Color kFieldColor{.red = 20, .green = 20, .blue = 20};
    constexpr Color kFocusedColor{.red = 30, .green = 30, .blue = 30};
    constexpr Color kCaretColor{.red = 250, .green = 250, .blue = 250};

    constexpr WidgetId kNameWidget{7};

    constexpr Size kCanvasSize{.width = 100, .height = 50};

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

}

TEST(TextFieldTest, TextField_DrawsItsCharactersOverTheUnfocusedFill)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textField(TextFieldSpec{.text = "ab"});

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ(kFieldColor, std::get<FillRect>(commands.at(0)).color);
    EXPECT_EQ("ab", std::get<DrawText>(commands.at(1)).text);
    EXPECT_EQ(kInkColor, std::get<DrawText>(commands.at(1)).color);
}

TEST(TextFieldTest, TextField_ShowsThePlaceholderMutedWhileItIsEmpty)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textField(TextFieldSpec{.placeholder = "name"});

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ("name", std::get<DrawText>(commands.at(1)).text);
    EXPECT_EQ(kMutedColor, std::get<DrawText>(commands.at(1)).color);
}

TEST(TextFieldTest, TextField_DrawsNothingButItsBoxWhileEmptyAndUnnamed)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textField(TextFieldSpec{});

    EXPECT_EQ(1U, uiContext.build().drawList.size());
}

TEST(TextFieldTest, TextField_AFocusedFieldTakesTheFocusedFillAndACaret)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textField(TextFieldSpec{.text = "ab", .focused = true});

    const auto commands = uiContext.build().drawList;

    EXPECT_EQ(kFocusedColor, std::get<FillRect>(commands.at(0)).color);
    ASSERT_EQ(1U, fillsColored(commands, kCaretColor).size());
}

TEST(TextFieldTest, TextField_TheCaretSitsAfterTheLastCharacterByDefault)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textField(TextFieldSpec{
        .text = "ab", .cursor = kCaretAtEnd, .focused = true});

    const auto commands = uiContext.build().drawList;
    const auto carets = fillsColored(commands, kCaretColor);

    ASSERT_EQ(1U, carets.size());

    EXPECT_EQ(12, carets.at(0).rect.originPoint.x);
    EXPECT_EQ(8U, carets.at(0).rect.size.height);
}

TEST(TextFieldTest, TextField_TheCaretSplitsTheTextWhereTheCursorIs)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textField(
        TextFieldSpec{.text = "abc", .cursor = 1, .focused = true});

    const auto commands = uiContext.build().drawList;
    const auto texts = textsOf(commands);

    ASSERT_EQ(2U, texts.size());
    EXPECT_EQ("a", texts.at(0));
    EXPECT_EQ("bc", texts.at(1));
    EXPECT_EQ(6, fillsColored(commands, kCaretColor).at(0).rect.originPoint.x);
}

TEST(TextFieldTest, TextField_TheCaretSitsAtTheStartOfAnEmptyFocusedField)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textField(
        TextFieldSpec{.placeholder = "name", .focused = true});

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(1U, fillsColored(commands, kCaretColor).size());
    EXPECT_EQ(0, fillsColored(commands, kCaretColor).at(0).rect.originPoint.x);
    EXPECT_EQ("name", textsOf(commands).at(0));
}

TEST(TextFieldTest, TextField_AnUnfocusedFieldReportsNoEditAtAll)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Character}, .typedText = "c"}};

    uiContext.textField(TextFieldSpec{.widgetId = kNameWidget, .text = "ab"});

    EXPECT_FALSE(uiContext.build().interactions.edit.has_value());
}

TEST(TextFieldTest, TextField_AQuietFrameReportsNoEditEither)
{
    Context uiContext{kCanvasSize, plainTheme(), Pointer{}, Keyboard{}};

    uiContext.textField(
        TextFieldSpec{.widgetId = kNameWidget, .text = "ab", .focused = true});

    EXPECT_FALSE(uiContext.build().interactions.edit.has_value());
}

TEST(TextFieldTest, TextField_TypingIsReportedAsTheTextItWouldMake)
{
    Context uiContext{kCanvasSize, plainTheme(), Pointer{}, Keyboard{
            .keys = {Key::Character, Key::Character}, .typedText = "cd"}};

    uiContext.textField(
        TextFieldSpec{.widgetId = kNameWidget, .text = "ab", .focused = true});

    const auto frame = uiContext.build();

    ASSERT_TRUE(frame.interactions.edit.has_value());
    EXPECT_EQ(kNameWidget, frame.interactions.edit->fieldWidget);
    EXPECT_EQ("abcd", frame.interactions.edit->text);
    EXPECT_EQ(4U, frame.interactions.edit->cursor);

    EXPECT_EQ("ab", textsOf(frame.drawList).at(0));
}

TEST(TextFieldTest, TextField_TypingGoesInWhereTheCursorIs)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Character}, .typedText = "X"}};

    uiContext.textField(TextFieldSpec{
        .widgetId = kNameWidget, .text = "ab", .cursor = 1, .focused = true});

    const auto edit = uiContext.build().interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ("aXb", edit->text);
    EXPECT_EQ(2U, edit->cursor);
}

TEST(TextFieldTest, TextField_BackspaceTakesTheCharacterBeforeTheCursor)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Backspace}}};

    uiContext.textField(
        TextFieldSpec{.widgetId = kNameWidget, .text = "abc", .focused = true});

    const auto edit = uiContext.build().interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ("ab", edit->text);
    EXPECT_EQ(2U, edit->cursor);
}

TEST(TextFieldTest, TextField_BackspaceAtTheStartChangesNothing)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Backspace}}};

    uiContext.textField(TextFieldSpec{
        .widgetId = kNameWidget, .text = "abc", .cursor = 0, .focused = true});

    EXPECT_FALSE(uiContext.build().interactions.edit.has_value());
}

TEST(TextFieldTest, TextField_TheArrowsWalkTheCursorOneCharacter)
{
    Context leftContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::MoveLeft}}};

    leftContext.textField(
        TextFieldSpec{.widgetId = kNameWidget, .text = "ab", .focused = true});

    const auto walkedInteractions = leftContext.build().interactions.edit;

    ASSERT_TRUE(walkedInteractions.has_value());
    EXPECT_EQ(1U, walkedInteractions->cursor);
    EXPECT_EQ("ab", walkedInteractions->text);

    Context rightContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::MoveRight}}};

    rightContext.textField(TextFieldSpec{
        .widgetId = kNameWidget, .text = "ab", .cursor = 0, .focused = true});

    const auto forward = rightContext.build().interactions.edit;

    ASSERT_TRUE(forward.has_value());
    EXPECT_EQ(1U, forward->cursor);
    EXPECT_EQ("ab", forward->text);
}

TEST(TextFieldTest, TextField_TheArrowsStopAtEitherEndOfTheText)
{
    Context startContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::MoveLeft}}};

    startContext.textField(TextFieldSpec{
        .widgetId = kNameWidget, .text = "ab", .cursor = 0, .focused = true});

    EXPECT_FALSE(startContext.build().interactions.edit.has_value());

    Context endContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::MoveRight}}};

    endContext.textField(
        TextFieldSpec{.widgetId = kNameWidget, .text = "ab", .focused = true});

    EXPECT_FALSE(endContext.build().interactions.edit.has_value());
}

TEST(TextFieldTest, TextField_EnterAndEscapeAreReportedWithoutChangingTheText)
{
    Context submitContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Activate}}};

    submitContext.textField(
        TextFieldSpec{.widgetId = kNameWidget, .text = "ab", .focused = true});

    const auto submittedInteractions = submitContext.build().interactions.edit;

    ASSERT_TRUE(submittedInteractions.has_value());
    EXPECT_TRUE(submittedInteractions->submitted);
    EXPECT_FALSE(submittedInteractions->cancelled);
    EXPECT_EQ("ab", submittedInteractions->text);

    Context cancelContext{
        kCanvasSize, plainTheme(), Pointer{}, Keyboard{.keys = {Key::Cancel}}};

    cancelContext.textField(
        TextFieldSpec{.widgetId = kNameWidget, .text = "ab", .focused = true});

    const auto cancelledInteractions = cancelContext.build().interactions.edit;

    ASSERT_TRUE(cancelledInteractions.has_value());
    EXPECT_TRUE(cancelledInteractions->cancelled);
}

TEST(TextFieldTest, TextField_ACursorPastTheEndIsTheEnd)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Character}, .typedText = "c"}};

    uiContext.textField(TextFieldSpec{
        .widgetId = kNameWidget, .text = "ab", .cursor = 99, .focused = true});

    const auto edit = uiContext.build().interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ("abc", edit->text);
}

TEST(TextFieldTest, TextField_APressOnTheBoxNamesTheField)
{
    const Pointer pointer{
        .positionPoint = Point{.x = 4, .y = 2}, .pressed = true};

    Context uiContext{kCanvasSize, plainTheme(), pointer};

    uiContext.textField(TextFieldSpec{.widgetId = kNameWidget, .text = "ab"});

    EXPECT_EQ(kNameWidget, uiContext.build().interactions.activatedWidget);
}

TEST(TextFieldTest, TextField_AFixedWidthFieldTakesExactlyThatMuch)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textField(
        TextFieldSpec{.widthSizing = fixedSize(30), .text = "ab"});

    const auto commands = uiContext.build().drawList;

    EXPECT_EQ(30U, std::get<FillRect>(commands.at(0)).rect.size.width);
}

TEST(TextFieldTest, TextField_ACaretIsAtLeastOnePixelWideAtAZeroScale)
{
    auto theme = plainTheme();
    theme.textScale = 0;

    Context uiContext{kCanvasSize, theme};

    uiContext.textField(TextFieldSpec{.text = "ab", .focused = true});

    const auto carets = fillsColored(uiContext.build().drawList, kCaretColor);

    ASSERT_EQ(1U, carets.size());
    EXPECT_EQ(1U, carets.at(0).rect.size.width);
}

TEST(TextFieldTest, TextField_TabReachesAFieldAndTheNextFrameTypesIntoIt)
{
    Context firstContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext}}};

    firstContext.textField(
        TextFieldSpec{.widgetId = kNameWidget, .text = "ab"});

    const auto firstFrame = firstContext.build();

    EXPECT_EQ(kNameWidget, firstFrame.interactions.focusedWidget);
    EXPECT_FALSE(firstFrame.interactions.edit.has_value());
    EXPECT_TRUE(fillsColored(firstFrame.drawList, kCaretColor).empty());

    Context secondContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Character}, .typedText = "c"},
        firstFrame.interactions.focusedWidget};

    secondContext.textField(
        TextFieldSpec{.widgetId = kNameWidget, .text = "ab"});

    const auto typedFrame = secondContext.build();

    ASSERT_TRUE(typedFrame.interactions.edit.has_value());
    EXPECT_EQ("abc", typedFrame.interactions.edit->text);

    EXPECT_EQ(1U, fillsColored(typedFrame.drawList, kCaretColor).size());
    EXPECT_EQ(kFocusedColor,
        std::get<FillRect>(typedFrame.drawList.at(0)).color);
}

TEST(TextFieldTest, TextField_EnterOnAFocusedFieldActivatesItAndSubmitsIt)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Activate}},
        kNameWidget};

    uiContext.textField(TextFieldSpec{.widgetId = kNameWidget, .text = "ab"});

    const auto frame = uiContext.build();

    EXPECT_EQ(kNameWidget, frame.interactions.activatedWidget);
    ASSERT_TRUE(frame.interactions.edit.has_value());
    EXPECT_TRUE(frame.interactions.edit->submitted);
    EXPECT_EQ("ab", frame.interactions.edit->text);
}

TEST(TextFieldTest, TextField_TabbingAwayLeavesTheCaretWhereItWas)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::FocusPrevious}},
        kNameWidget};

    uiContext.textField(TextFieldSpec{
        .widgetId = kNameWidget, .text = "ab", .cursor = 1, .focused = true});

    EXPECT_FALSE(uiContext.build().interactions.edit.has_value());
}

TEST(TextFieldTest, TextField_ACharacterAndABackspaceKeepTheOrderTheyArrivedIn)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{
            .keys = {Key::Character, Key::Backspace, Key::Character},
            .typedText = "ab"}};

    uiContext.textField(
        TextFieldSpec{.widgetId = kNameWidget, .text = "", .focused = true});

    const auto edit = uiContext.build().interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ("b", edit->text);
    EXPECT_EQ(1U, edit->cursor);
}

TEST(TextFieldTest, TextField_ACharacterWithNoEdgeToTakeItIsNotTyped)
{
    Context uiContext{
        kCanvasSize, plainTheme(), Pointer{}, Keyboard{.typedText = "c"}};

    uiContext.textField(
        TextFieldSpec{.widgetId = kNameWidget, .text = "ab", .focused = true});

    EXPECT_FALSE(uiContext.build().interactions.edit.has_value());
}

TEST(TextFieldTest, TextField_MoreCharacterEdgesThanCharactersTypeWhatThereIs)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{
            .keys = {Key::Character, Key::Character}, .typedText = "c"}};

    uiContext.textField(
        TextFieldSpec{.widgetId = kNameWidget, .text = "ab", .focused = true});

    const auto edit = uiContext.build().interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ("abc", edit->text);
}

TEST(TextFieldTest, TextField_AFieldIsOneLineSoTheVerticalKeysDoNothing)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::MoveUp, Key::MoveDown}}};

    uiContext.textField(
        TextFieldSpec{.widgetId = kNameWidget, .text = "ab", .focused = true});

    EXPECT_FALSE(uiContext.build().interactions.edit.has_value());
}

TEST(TextFieldTest, TextField_AFieldsOneLineIsWhatHomeAndEndMoveOver)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::MoveLineStart}}};

    uiContext.textField(
        TextFieldSpec{
            .widgetId = kNameWidget,
            .text = "abc",
            .cursor = 2,
            .focused = true});

    const auto edit = uiContext.build().interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ(0U, edit->cursor);

    Context againContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::MoveLineEnd}}};

    againContext.textField(
        TextFieldSpec{
            .widgetId = kNameWidget,
            .text = "abc",
            .cursor = 1,
            .focused = true});

    const auto end = againContext.build().interactions.edit;

    ASSERT_TRUE(end.has_value());
    EXPECT_EQ(3U, end->cursor);
}
