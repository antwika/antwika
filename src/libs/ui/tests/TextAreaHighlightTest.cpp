#include <gtest/gtest.h>

#include <array>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/support/DrawListQueries.hpp>

#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/Frame.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Size;
using antwika::ui::support::fillsColored;
using antwika::ui::support::textsOf;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::Frame;
using antwika::ui::TextAreaSpec;
using antwika::ui::TextHighlight;
using antwika::ui::Theme;
using antwika::widget::WidgetId;

namespace
{
    constexpr Color kInkColor{.red = 200, .green = 210, .blue = 220};
    constexpr Color kPickedColor{.red = 40, .green = 70, .blue = 120};
    constexpr Color kLitColor{.red = 30, .green = 90, .blue = 50};

    constexpr WidgetId kCodeWidget{9};

    constexpr Size kCanvasSize{.width = 200, .height = 100};

    Theme plainTheme()
    {
        return Theme{
            .textColor = kInkColor,
            .selectionColor = kPickedColor,
            .highlightColor = kLitColor,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    [[nodiscard]] Frame frameOf(TextAreaSpec spec)
    {
        spec.widgetId = kCodeWidget;

        Context uiContext{kCanvasSize, plainTheme()};

        uiContext.textArea(spec);

        return uiContext.build();
    }

    [[nodiscard]] std::vector<std::string> piecesOn(
        const DrawList &drawList, const Color groundColor)
    {
        std::vector<std::string> pieces;
        std::optional<FillRect> behindRect;

        for (const auto &command : drawList)
        {
            if (const auto *fill = std::get_if<FillRect>(&command))
            {
                behindRect = fill->color == groundColor ? std::optional{*fill}
                           : std::nullopt;

                continue;
            }

            const auto *text = std::get_if<DrawText>(&command);

            if (text != nullptr && behindRect.has_value()
                && behindRect->rect.originPoint.x == text->originPoint.x
                && behindRect->rect.originPoint.y == text->originPoint.y)
            {
                pieces.push_back(text->text);
            }
        }

        return pieces;
    }
}

TEST(TextAreaHighlightTest, TextArea_ASpanShowsAsItsOwnGround)
{
    constexpr std::array<TextHighlight, 1> highlightSpans{
        TextHighlight{.begin = 2, .end = 5}};

    const auto frame =
        frameOf(TextAreaSpec{.text = "abcdefgh", .highlights = highlightSpans});

    const auto litPieces = piecesOn(frame.drawList, kLitColor);

    ASSERT_EQ(litPieces.size(), 1U);
    EXPECT_EQ(litPieces[0], "cde");
}

TEST(TextAreaHighlightTest, TextArea_TwoSpansLightTwoPieces)
{
    constexpr std::array<TextHighlight, 2> highlightSpans{
        TextHighlight{.begin = 0, .end = 1},
        TextHighlight{.begin = 4, .end = 6}};

    const auto frame =
        frameOf(TextAreaSpec{.text = "abcdefgh", .highlights = highlightSpans});

    const auto litPieces = piecesOn(frame.drawList, kLitColor);

    ASSERT_EQ(litPieces.size(), 2U);
    EXPECT_EQ(litPieces[0], "a");
    EXPECT_EQ(litPieces[1], "ef");
}

TEST(TextAreaHighlightTest, TextArea_TheSelectionWinsWhereTheyOverlap)
{
    constexpr std::array<TextHighlight, 1> highlightSpans{
        TextHighlight{.begin = 0, .end = 6}};

    auto spec = TextAreaSpec{
        .text = "abcdefgh",
        .cursor = 2,
        .anchor = 4,
        .highlights = highlightSpans,
        .focused = true};

    const auto frame = frameOf(spec);

    const auto pickedPieces = piecesOn(frame.drawList, kPickedColor);
    const auto litPieces = piecesOn(frame.drawList, kLitColor);

    ASSERT_EQ(pickedPieces.size(), 1U);
    EXPECT_EQ(pickedPieces[0], "cd");

    ASSERT_EQ(litPieces.size(), 2U);
    EXPECT_EQ(litPieces[0], "ab");
    EXPECT_EQ(litPieces[1], "ef");
}

TEST(TextAreaHighlightTest, TextArea_ASpanAcrossABreakLightsBothLines)
{
    constexpr std::array<TextHighlight, 1> highlightSpans{
        TextHighlight{.begin = 1, .end = 6}};

    const auto frame =
        frameOf(TextAreaSpec{.text = "abc\ndef", .highlights = highlightSpans});

    const auto litPieces = piecesOn(frame.drawList, kLitColor);

    ASSERT_EQ(litPieces.size(), 2U);
    EXPECT_EQ(litPieces[0], "bc");
    EXPECT_EQ(litPieces[1], "de");
}

TEST(TextAreaHighlightTest, TextArea_EndsPastTheTextAreClamped)
{
    constexpr std::array<TextHighlight, 1> highlightSpans{
        TextHighlight{.begin = 2, .end = 99}};

    const auto frame =
        frameOf(TextAreaSpec{.text = "abcd", .highlights = highlightSpans});

    const auto litPieces = piecesOn(frame.drawList, kLitColor);

    ASSERT_EQ(litPieces.size(), 1U);
    EXPECT_EQ(litPieces[0], "cd");
}

TEST(TextAreaHighlightTest, TextArea_ASpanOfNothingLightsNothing)
{
    constexpr std::array<TextHighlight, 2> highlightSpans{
        TextHighlight{.begin = 2, .end = 2},
        TextHighlight{.begin = 99, .end = 120}};

    const auto frame =
        frameOf(TextAreaSpec{.text = "abcd", .highlights = highlightSpans});

    EXPECT_TRUE(piecesOn(frame.drawList, kLitColor).empty());
}

TEST(TextAreaHighlightTest, TextArea_ASpanOfNothingCutsNoPieceOffTheLine)
{
    constexpr std::array<TextHighlight, 1> highlightSpans{
        TextHighlight{.begin = 2, .end = 2}};

    const auto frame =
        frameOf(TextAreaSpec{.text = "abcd", .highlights = highlightSpans});

    EXPECT_EQ(
        textsOf(frame.drawList), (std::vector<std::string>{"abcd"}));
}

TEST(TextAreaHighlightTest, TextArea_AHighlightAloneReportsNoEdit)
{
    constexpr std::array<TextHighlight, 1> highlightSpans{
        TextHighlight{.begin = 0, .end = 2}};

    const auto frame = frameOf(TextAreaSpec{
        .text = "abcd", .highlights = highlightSpans, .focused = true});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}
