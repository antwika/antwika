#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/TextInput.hpp"
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
using antwika::ui::kCaretAtEnd;
using antwika::ui::Pointer;
using antwika::ui::TextFieldSpec;
using antwika::ui::TextInput;
using antwika::ui::Theme;
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kInk{.red = 200, .green = 210, .blue = 220};
    constexpr Color kMuted{.red = 100, .green = 110, .blue = 105};
    constexpr Color kField{.red = 20, .green = 20, .blue = 20};
    constexpr Color kFocused{.red = 30, .green = 30, .blue = 30};
    constexpr Color kCaret{.red = 250, .green = 250, .blue = 250};

    constexpr WidgetId kName{7};

    constexpr Size kCanvas{.width = 100, .height = 50};

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
} // namespace

TEST(TextFieldTest, DrawsItsCharactersOverTheUnfocusedFill)
{
    Context ui{kCanvas, plainTheme()};

    ui.textField(TextFieldSpec{.text = "ab"});

    const auto commands = ui.finish().commands;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ(kField, std::get<FillRect>(commands.at(0)).color);
    EXPECT_EQ("ab", std::get<DrawText>(commands.at(1)).text);
    EXPECT_EQ(kInk, std::get<DrawText>(commands.at(1)).color);
}

TEST(TextFieldTest, ShowsThePlaceholderMutedWhileItIsEmpty)
{
    Context ui{kCanvas, plainTheme()};

    ui.textField(TextFieldSpec{.placeholder = "name"});

    const auto commands = ui.finish().commands;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ("name", std::get<DrawText>(commands.at(1)).text);
    EXPECT_EQ(kMuted, std::get<DrawText>(commands.at(1)).color);
}

TEST(TextFieldTest, DrawsNothingButItsBoxWhileEmptyAndUnnamed)
{
    Context ui{kCanvas, plainTheme()};

    ui.textField(TextFieldSpec{});

    EXPECT_EQ(1U, ui.finish().commands.size());
}

TEST(TextFieldTest, AFocusedFieldTakesTheFocusedFillAndACaret)
{
    Context ui{kCanvas, plainTheme()};

    ui.textField(TextFieldSpec{.text = "ab", .focused = true});

    const auto commands = ui.finish().commands;

    EXPECT_EQ(kFocused, std::get<FillRect>(commands.at(0)).color);
    ASSERT_EQ(1U, caretsOf(commands).size());
}

TEST(TextFieldTest, TheCaretSitsAfterTheLastCharacterByDefault)
{
    Context ui{kCanvas, plainTheme()};

    ui.textField(TextFieldSpec{
        .text = "ab", .cursor = kCaretAtEnd, .focused = true});

    const auto commands = ui.finish().commands;
    const auto carets = caretsOf(commands);

    ASSERT_EQ(1U, carets.size());

    // Two glyph cells wide, at scale one.
    EXPECT_EQ(12, carets.at(0).rect.origin.x);
    EXPECT_EQ(8U, carets.at(0).rect.size.height);
}

TEST(TextFieldTest, TheCaretSplitsTheTextWhereTheCursorIs)
{
    Context ui{kCanvas, plainTheme()};

    ui.textField(
        TextFieldSpec{.text = "abc", .cursor = 1, .focused = true});

    const auto commands = ui.finish().commands;
    const auto texts = textsOf(commands);

    ASSERT_EQ(2U, texts.size());
    EXPECT_EQ("a", texts.at(0));
    EXPECT_EQ("bc", texts.at(1));
    EXPECT_EQ(6, caretsOf(commands).at(0).rect.origin.x);
}

TEST(TextFieldTest, TheCaretSitsAtTheStartOfAnEmptyFocusedField)
{
    Context ui{kCanvas, plainTheme()};

    ui.textField(
        TextFieldSpec{.placeholder = "name", .focused = true});

    const auto commands = ui.finish().commands;

    ASSERT_EQ(1U, caretsOf(commands).size());
    EXPECT_EQ(0, caretsOf(commands).at(0).rect.origin.x);
    EXPECT_EQ("name", textsOf(commands).at(0));
}

TEST(TextFieldTest, AnUnfocusedFieldReportsNoEditAtAll)
{
    Context ui{kCanvas, plainTheme(), Pointer{}, TextInput{.typed = "c"}};

    ui.textField(TextFieldSpec{.id = kName, .text = "ab"});

    EXPECT_FALSE(ui.finish().interactions.edit.has_value());
}

TEST(TextFieldTest, AQuietFrameReportsNoEditEither)
{
    Context ui{kCanvas, plainTheme(), Pointer{}, TextInput{}};

    ui.textField(
        TextFieldSpec{.id = kName, .text = "ab", .focused = true});

    EXPECT_FALSE(ui.finish().interactions.edit.has_value());
}

TEST(TextFieldTest, TypingIsReportedAsTheTextItWouldMake)
{
    Context ui{kCanvas, plainTheme(), Pointer{}, TextInput{.typed = "cd"}};

    ui.textField(
        TextFieldSpec{.id = kName, .text = "ab", .focused = true});

    const auto frame = ui.finish();

    ASSERT_TRUE(frame.interactions.edit.has_value());
    EXPECT_EQ(kName, frame.interactions.edit->field);
    EXPECT_EQ("abcd", frame.interactions.edit->text);
    EXPECT_EQ(4U, frame.interactions.edit->cursor);

    // The picture predates the edit, as a button's does.
    EXPECT_EQ("ab", textsOf(frame.commands).at(0));
}

TEST(TextFieldTest, TypingGoesInWhereTheCursorIs)
{
    Context ui{kCanvas, plainTheme(), Pointer{}, TextInput{.typed = "X"}};

    ui.textField(TextFieldSpec{
        .id = kName, .text = "ab", .cursor = 1, .focused = true});

    const auto edit = ui.finish().interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ("aXb", edit->text);
    EXPECT_EQ(2U, edit->cursor);
}

TEST(TextFieldTest, BackspaceTakesTheCharacterBeforeTheCursor)
{
    Context ui{
        kCanvas, plainTheme(), Pointer{}, TextInput{.backspace = true}};

    ui.textField(
        TextFieldSpec{.id = kName, .text = "abc", .focused = true});

    const auto edit = ui.finish().interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ("ab", edit->text);
    EXPECT_EQ(2U, edit->cursor);
}

TEST(TextFieldTest, BackspaceAtTheStartChangesNothing)
{
    Context ui{
        kCanvas, plainTheme(), Pointer{}, TextInput{.backspace = true}};

    ui.textField(TextFieldSpec{
        .id = kName, .text = "abc", .cursor = 0, .focused = true});

    EXPECT_FALSE(ui.finish().interactions.edit.has_value());
}

TEST(TextFieldTest, TheArrowsWalkTheCursorAndStopAtTheEnds)
{
    Context left{
        kCanvas, plainTheme(), Pointer{}, TextInput{.left = true}};

    left.textField(
        TextFieldSpec{.id = kName, .text = "ab", .focused = true});

    const auto walked = left.finish().interactions.edit;

    ASSERT_TRUE(walked.has_value());
    EXPECT_EQ(1U, walked->cursor);
    EXPECT_EQ("ab", walked->text);

    Context start{
        kCanvas, plainTheme(), Pointer{}, TextInput{.left = true}};

    start.textField(TextFieldSpec{
        .id = kName, .text = "ab", .cursor = 0, .focused = true});

    EXPECT_FALSE(start.finish().interactions.edit.has_value());

    Context right{
        kCanvas, plainTheme(), Pointer{}, TextInput{.right = true}};

    right.textField(TextFieldSpec{
        .id = kName, .text = "ab", .cursor = 0, .focused = true});

    const auto forward = right.finish().interactions.edit;

    ASSERT_TRUE(forward.has_value());
    EXPECT_EQ(1U, forward->cursor);

    Context end{
        kCanvas, plainTheme(), Pointer{}, TextInput{.right = true}};

    end.textField(
        TextFieldSpec{.id = kName, .text = "ab", .focused = true});

    EXPECT_FALSE(end.finish().interactions.edit.has_value());
}

TEST(TextFieldTest, EnterAndEscapeAreReportedWithoutChangingTheText)
{
    Context submit{
        kCanvas, plainTheme(), Pointer{}, TextInput{.submit = true}};

    submit.textField(
        TextFieldSpec{.id = kName, .text = "ab", .focused = true});

    const auto submitted = submit.finish().interactions.edit;

    ASSERT_TRUE(submitted.has_value());
    EXPECT_TRUE(submitted->submitted);
    EXPECT_FALSE(submitted->cancelled);
    EXPECT_EQ("ab", submitted->text);

    Context cancel{
        kCanvas, plainTheme(), Pointer{}, TextInput{.cancel = true}};

    cancel.textField(
        TextFieldSpec{.id = kName, .text = "ab", .focused = true});

    const auto cancelled = cancel.finish().interactions.edit;

    ASSERT_TRUE(cancelled.has_value());
    EXPECT_TRUE(cancelled->cancelled);
}

TEST(TextFieldTest, ACursorPastTheEndIsTheEnd)
{
    Context ui{kCanvas, plainTheme(), Pointer{}, TextInput{.typed = "c"}};

    ui.textField(TextFieldSpec{
        .id = kName, .text = "ab", .cursor = 99, .focused = true});

    const auto edit = ui.finish().interactions.edit;

    ASSERT_TRUE(edit.has_value());
    EXPECT_EQ("abc", edit->text);
}

TEST(TextFieldTest, APressOnTheBoxNamesTheField)
{
    const Pointer pointer{
        .position = Point{.x = 4, .y = 2}, .pressed = true};

    Context ui{kCanvas, plainTheme(), pointer};

    ui.textField(TextFieldSpec{.id = kName, .text = "ab"});

    EXPECT_EQ(kName, ui.finish().interactions.activated);
}

TEST(TextFieldTest, AFixedWidthFieldTakesExactlyThatMuch)
{
    Context ui{kCanvas, plainTheme()};

    ui.textField(
        TextFieldSpec{.width = fixedSize(30), .text = "ab"});

    const auto commands = ui.finish().commands;

    EXPECT_EQ(30U, std::get<FillRect>(commands.at(0)).rect.size.width);
}

TEST(TextFieldTest, ACaretIsAtLeastOnePixelWideAtAZeroScale)
{
    auto theme = plainTheme();
    theme.textScale = 0;

    Context ui{kCanvas, theme};

    ui.textField(TextFieldSpec{.text = "ab", .focused = true});

    const auto carets = caretsOf(ui.finish().commands);

    ASSERT_EQ(1U, carets.size());
    EXPECT_EQ(1U, carets.at(0).rect.size.width);
}
