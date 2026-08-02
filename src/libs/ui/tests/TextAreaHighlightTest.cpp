#include <array>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/Frame.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Size;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::Frame;
using antwika::ui::TextAreaSpec;
using antwika::ui::TextHighlight;
using antwika::ui::Theme;
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kInk{.red = 200, .green = 210, .blue = 220};
    constexpr Color kPicked{.red = 40, .green = 70, .blue = 120};
    constexpr Color kLit{.red = 30, .green = 90, .blue = 50};

    constexpr WidgetId kCode{9};

    constexpr Size kCanvas{.width = 200, .height = 100};

    Theme plainTheme()
    {
        return Theme{
            .text = kInk,
            .selection = kPicked,
            .highlight = kLit,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    [[nodiscard]] Frame frameOf(TextAreaSpec spec)
    {
        spec.id = kCode;

        Context ui{kCanvas, plainTheme()};

        ui.textArea(spec);

        return ui.finish();
    }

    // Every piece of text drawn on a given ground, in drawing order.
    // A piece is wholly on a ground or not at all.
    // So this is the whole answer rather than a sample of it.
    [[nodiscard]] std::vector<std::string> piecesOn(
        const DrawList &commands, const Color ground)
    {
        std::vector<std::string> pieces;
        std::optional<FillRect> behind;

        for (const auto &command : commands)
        {
            if (const auto *fill = std::get_if<FillRect>(&command))
            {
                behind = fill->color == ground ? std::optional{*fill}
                                               : std::nullopt;

                continue;
            }

            const auto *text = std::get_if<DrawText>(&command);

            if (text != nullptr && behind.has_value()
                && behind->rect.origin.x == text->origin.x
                && behind->rect.origin.y == text->origin.y)
            {
                pieces.push_back(text->text);
            }
        }

        return pieces;
    }
} // namespace

TEST(TextAreaHighlightTest, ASpanShowsAsItsOwnGround)
{
    constexpr std::array<TextHighlight, 1> spans{
        TextHighlight{.begin = 2, .end = 5}};

    const auto frame =
        frameOf(TextAreaSpec{.text = "abcdefgh", .highlights = spans});

    const auto lit = piecesOn(frame.commands, kLit);

    ASSERT_EQ(lit.size(), 1U);
    EXPECT_EQ(lit[0], "cde");
}

TEST(TextAreaHighlightTest, TwoSpansLightTwoPieces)
{
    constexpr std::array<TextHighlight, 2> spans{
        TextHighlight{.begin = 0, .end = 1},
        TextHighlight{.begin = 4, .end = 6}};

    const auto frame =
        frameOf(TextAreaSpec{.text = "abcdefgh", .highlights = spans});

    const auto lit = piecesOn(frame.commands, kLit);

    ASSERT_EQ(lit.size(), 2U);
    EXPECT_EQ(lit[0], "a");
    EXPECT_EQ(lit[1], "ef");
}

// The selection is what the next keystroke acts on.
// So it wins the ground wherever the two overlap.
TEST(TextAreaHighlightTest, TheSelectionWinsWhereTheyOverlap)
{
    constexpr std::array<TextHighlight, 1> spans{
        TextHighlight{.begin = 0, .end = 6}};

    auto spec = TextAreaSpec{
        .text = "abcdefgh",
        .cursor = 2,
        .anchor = 4,
        .highlights = spans,
        .focused = true};

    const auto frame = frameOf(spec);

    const auto picked = piecesOn(frame.commands, kPicked);
    const auto lit = piecesOn(frame.commands, kLit);

    ASSERT_EQ(picked.size(), 1U);
    EXPECT_EQ(picked[0], "cd");

    ASSERT_EQ(lit.size(), 2U);
    EXPECT_EQ(lit[0], "ab");
    EXPECT_EQ(lit[1], "ef");
}

TEST(TextAreaHighlightTest, ASpanAcrossABreakLightsBothLines)
{
    constexpr std::array<TextHighlight, 1> spans{
        TextHighlight{.begin = 1, .end = 6}};

    const auto frame =
        frameOf(TextAreaSpec{.text = "abc\ndef", .highlights = spans});

    const auto lit = piecesOn(frame.commands, kLit);

    ASSERT_EQ(lit.size(), 2U);
    EXPECT_EQ(lit[0], "bc");
    EXPECT_EQ(lit[1], "de");
}

// Ends past the text are brought inside, as the caret's is.
TEST(TextAreaHighlightTest, EndsPastTheTextAreClamped)
{
    constexpr std::array<TextHighlight, 1> spans{
        TextHighlight{.begin = 2, .end = 99}};

    const auto frame =
        frameOf(TextAreaSpec{.text = "abcd", .highlights = spans});

    const auto lit = piecesOn(frame.commands, kLit);

    ASSERT_EQ(lit.size(), 1U);
    EXPECT_EQ(lit[0], "cd");
}

TEST(TextAreaHighlightTest, ASpanOfNothingLightsNothing)
{
    constexpr std::array<TextHighlight, 2> spans{
        TextHighlight{.begin = 2, .end = 2},
        TextHighlight{.begin = 99, .end = 120}};

    const auto frame =
        frameOf(TextAreaSpec{.text = "abcd", .highlights = spans});

    EXPECT_TRUE(piecesOn(frame.commands, kLit).empty());
}

// A highlight is a change of ground and nothing else.
// It moves no caret and reports no edit.
TEST(TextAreaHighlightTest, AHighlightAloneReportsNoEdit)
{
    constexpr std::array<TextHighlight, 1> spans{
        TextHighlight{.begin = 0, .end = 2}};

    const auto frame = frameOf(TextAreaSpec{
        .text = "abcd", .highlights = spans, .focused = true});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}
