#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Frame.hpp"
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/ScrollChange.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::ui::Context;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::Frame;
using antwika::ui::Key;
using antwika::ui::Keyboard;
using antwika::ui::kNoWidget;
using antwika::ui::LineBand;
using antwika::ui::Pointer;
using antwika::ui::ScrollChange;
using antwika::ui::TextAreaSpec;
using antwika::ui::Theme;
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kInk{.red = 200, .green = 210, .blue = 220};
    constexpr Color kThumb{.red = 78, .green = 86, .blue = 106};

    constexpr WidgetId kCode{9};
    constexpr WidgetId kBand{40};
    constexpr WidgetId kOther{41};

    // Eight rows of room at this scale, as the scroll suite has.
    constexpr Size kCanvas{.width = 200, .height = 64};
    constexpr std::uint32_t kLineHeight = 8;

    Theme plainTheme()
    {
        return Theme{
            .text = kInk,
            .scrollThumb = kThumb,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0,
            .scrollbarWidth = 8};
    }

    /**
     * @brief Get a document of one-character lines.
     * @param lines How many.
     * @return The document, with no break after the last line.
     */
    [[nodiscard]] std::string linesOf(std::size_t lines)
    {
        std::string text;

        for (std::size_t at = 0; at < lines; ++at)
        {
            text += static_cast<char>('a' + (at % 26));

            if (at + 1 < lines)
            {
                text += '\n';
            }
        }

        return text;
    }

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

    [[nodiscard]] std::int32_t topOf(
        const Frame &frame, const std::string &text)
    {
        for (const auto &command : frame.commands)
        {
            const auto *drawn = std::get_if<DrawText>(&command);

            if (drawn != nullptr && drawn->text == text)
            {
                return drawn->origin.y;
            }
        }

        return -1;
    }

    [[nodiscard]] std::vector<FillRect> fillsOf(
        const Frame &frame, Color color)
    {
        std::vector<FillRect> fills;

        for (const auto &command : frame.commands)
        {
            const auto *fill = std::get_if<FillRect>(&command);

            if (fill != nullptr && fill->color == color)
            {
                fills.push_back(*fill);
            }
        }

        return fills;
    }
} // namespace

// The room is held open under the line the band names.
// Named, the band says where it went through Frame::rects.
// And everything beneath it moves down by what it holds.
TEST(TextAreaBandTest, ABandHoldsRoomOpenBeneathItsLine)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 1, .rows = 2, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bands = bands});

    const auto found = frame.rects.find(kBand);

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(
        *found,
        (Rect{
            .origin = {.x = 0, .y = 2 * kLineHeight},
            .size = {.width = kCanvas.width, .height = 2 * kLineHeight}}));

    // The line beneath starts under the band rather than under its line.
    EXPECT_EQ(topOf(frame, "c"), 4 * kLineHeight);
}

TEST(TextAreaBandTest, ABandOfNoRowsHoldsNothing)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 1, .rows = 0, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bands = bands});

    EXPECT_FALSE(frame.rects.find(kBand).has_value());
    EXPECT_EQ(topOf(frame, "c"), 2 * kLineHeight);
}

TEST(TextAreaBandTest, ABandNamingALineTheTextDoesNotHaveHoldsNothing)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 50, .rows = 2, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bands = bands});

    EXPECT_FALSE(frame.rects.find(kBand).has_value());
}

TEST(TextAreaBandTest, TwoBandsOnOneLineStackInDeclarationOrder)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 0, .rows = 1, .id = kBand},
        LineBand{.line = 0, .rows = 1, .id = kOther}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bands = bands});

    const auto first = frame.rects.find(kBand);
    const auto second = frame.rects.find(kOther);

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->origin.y, 1 * kLineHeight);
    EXPECT_EQ(second->origin.y, 2 * kLineHeight);

    // Two bands of one line are two values, however they stack.
    // A band differs by any of its line, its rows or its id.
    EXPECT_NE(bands[0], bands[1]);
    EXPECT_NE(bands[0], (LineBand{.line = 1, .rows = 1, .id = kBand}));
    EXPECT_NE(bands[0], (LineBand{.line = 0, .rows = 2, .id = kBand}));
    EXPECT_EQ(bands[0], (LineBand{.line = 0, .rows = 1, .id = kBand}));
}

// A band whose line is scrolled off the top is off with it.
// So a caller reading Frame::rects draws nothing for it.
TEST(TextAreaBandTest, ABandScrolledOffTheTopIsOffWithIt)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 1, .rows = 2, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(20),
            .cursor = 0,
            .scroll = 2,
            .bands = bands});

    EXPECT_FALSE(frame.rects.find(kBand).has_value());
}

// An unnamed band still holds its room.
// There is simply no way to ask where it went.
TEST(TextAreaBandTest, AnUnnamedBandStillHoldsItsRoom)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 0, .rows = 1, .id = kNoWidget}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bands = bands});

    EXPECT_EQ(topOf(frame, "b"), 2 * kLineHeight);
}

// The click landed on what was on the screen.
// A line under a band is drawn further down, and is hit further down.
TEST(TextAreaBandTest, AClickBelowABandLandsOnTheLineThatWasUnderIt)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 0, .rows = 2, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bands = bands},
        Keyboard{},
        Pointer{
            .position = Point{.x = 0, .y = 3 * kLineHeight + 1},
            .down = true,
            .pressed = true});

    ASSERT_TRUE(frame.interactions.edit.has_value());

    // The fourth row is the second line, not the fourth.
    EXPECT_EQ(frame.interactions.edit->cursor, 2U);
}

TEST(TextAreaBandTest, AClickInsideABandBelongsToTheLineItHangsUnder)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 0, .rows = 2, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bands = bands},
        Keyboard{},
        Pointer{
            .position = Point{.x = 40, .y = kLineHeight + 1},
            .down = true,
            .pressed = true});

    ASSERT_TRUE(frame.interactions.edit.has_value());

    // The band's line's end, since the click was past its one letter.
    EXPECT_EQ(frame.interactions.edit->cursor, 1U);
}

TEST(TextAreaBandTest, AClickBelowEveryLineIsTheEndOfTheDocument)
{
    const auto text = linesOf(4);

    const std::vector<LineBand> bands{
        LineBand{.line = 0, .rows = 2, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{.text = text, .cursor = 0, .bands = bands},
        Keyboard{},
        Pointer{
            .position = Point{.x = 0, .y = 7 * kLineHeight + 1},
            .down = true,
            .pressed = true});

    ASSERT_TRUE(frame.interactions.edit.has_value());
    EXPECT_EQ(frame.interactions.edit->cursor, text.size());
}

// How far the pane can scroll counts the bands' room as content.
// Six lines fit a page of eight rows; six lines and a band do not.
TEST(TextAreaBandTest, AskingForALineTooFarDownCountsTheBandsRoom)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 3, .rows = 4, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(6),
            .cursor = 0,
            .scroll = 500,
            .bands = bands});

    EXPECT_EQ(
        frame.interactions.scrolled,
        (ScrollChange{.area = kCode, .line = 2}));
}

// A band may outgrow the page it is shown in.
// The furthest top line is then still a real line.
TEST(TextAreaBandTest, ABandTallerThanThePageStillLeavesARealTopLine)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 1, .rows = 20, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(2),
            .cursor = 0,
            .scroll = 500,
            .bands = bands});

    EXPECT_EQ(
        frame.interactions.scrolled,
        (ScrollChange{.area = kCode, .line = 1}));
}

// A caret brought into view counts the bands above it too.
// Eight rows hold the caret's line and the banded one three above it.
TEST(TextAreaBandTest, TypingBelowThePaneCountsTheBandsRoomAboveTheCaret)
{
    const auto text = linesOf(20);

    const std::vector<LineBand> bands{
        LineBand{.line = 16, .rows = 4, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{
            .text = text,
            .cursor = text.size(),
            .scroll = 0,
            .bands = bands},
        Keyboard{.keys = {Key::Character}, .typed = "X"});

    ASSERT_TRUE(frame.interactions.scrolled.has_value());
    EXPECT_EQ(frame.interactions.scrolled->line, 16U);
}

// As long a share of the track as is showing of the rows.
// Sixteen lines and a sixteen-row band are four pages of content.
TEST(TextAreaBandTest, TheThumbCountsABandAsContent)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 0, .rows = 16, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(16),
            .cursor = 0,
            .bands = bands,
            .scrollbar = true});

    const auto thumbs = fillsOf(frame, kThumb);

    ASSERT_EQ(thumbs.size(), 1U);
    EXPECT_EQ(thumbs[0].rect.size.height, kCanvas.height / 4);
}

TEST(TextAreaBandTest, TheThumbSitsAsFarDownAsTheRowsDo)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 0, .rows = 16, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(16),
            .cursor = 0,
            .scroll = 500,
            .bands = bands,
            .scrollbar = true});

    const auto thumbs = fillsOf(frame, kThumb);

    ASSERT_EQ(thumbs.size(), 1U);

    // The last page, so the thumb ends where the track does.
    EXPECT_EQ(
        thumbs[0].rect.origin.y
            + static_cast<std::int32_t>(thumbs[0].rect.size.height),
        static_cast<std::int32_t>(kCanvas.height));
}
