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

#include "LinesOf.hpp"
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
using antwika::ui::tests::linesOf;

namespace
{
    constexpr Color kInk{.red = 200, .green = 210, .blue = 220};
    constexpr Color kThumb{.red = 78, .green = 86, .blue = 106};

    constexpr WidgetId kCode{9};
    constexpr WidgetId kBand{40};
    constexpr WidgetId kOther{41};

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
}

TEST(TextAreaBandTest, TextArea_ABandHoldsRoomOpenBeneathItsLine)
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

    EXPECT_EQ(topOf(frame, "c"), 4 * kLineHeight);
}

TEST(TextAreaBandTest, TextArea_ABandOfNoRowsHoldsNothing)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 1, .rows = 0, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bands = bands});

    EXPECT_FALSE(frame.rects.find(kBand).has_value());
    EXPECT_EQ(topOf(frame, "c"), 2 * kLineHeight);
}

TEST(TextAreaBandTest, TextArea_ABandNamingALineTheTextDoesNotHaveHoldsNothing)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 50, .rows = 2, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bands = bands});

    EXPECT_FALSE(frame.rects.find(kBand).has_value());
}

TEST(TextAreaBandTest, TextArea_TwoBandsOnOneLineStackInDeclarationOrder)
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

    EXPECT_NE(bands[0], bands[1]);
    EXPECT_NE(bands[0], (LineBand{.line = 1, .rows = 1, .id = kBand}));
    EXPECT_NE(bands[0], (LineBand{.line = 0, .rows = 2, .id = kBand}));
    EXPECT_EQ(bands[0], (LineBand{.line = 0, .rows = 1, .id = kBand}));
}

TEST(TextAreaBandTest, TextArea_ABandScrolledOffTheTopIsOffWithIt)
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

TEST(TextAreaBandTest, TextArea_AnUnnamedBandStillHoldsItsRoom)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 0, .rows = 1, .id = kNoWidget}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bands = bands});

    EXPECT_EQ(topOf(frame, "b"), 2 * kLineHeight);
}

TEST(TextAreaBandTest, TextArea_AClickBelowABandLandsOnTheLineThatWasUnderIt)
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

    EXPECT_EQ(frame.interactions.edit->cursor, 2U);
}

TEST(TextAreaBandTest, TextArea_AClickInsideABandBelongsToTheLineItHangsUnder)
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

    EXPECT_EQ(frame.interactions.edit->cursor, 1U);
}

TEST(TextAreaBandTest, TextArea_AClickBelowEveryLineIsTheEndOfTheDocument)
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

TEST(TextAreaBandTest, TextArea_AskingForALineTooFarDownCountsTheBandsRoom)
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

TEST(TextAreaBandTest, TextArea_ABandDecidesHowFarDownThePaneCanGo)
{
    const std::vector<LineBand> bands{
        LineBand{.line = 0, .rows = 6, .id = kBand}};

    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(4),
            .cursor = 0,
            .scroll = 500,
            .bands = bands});

    EXPECT_EQ(
        frame.interactions.scrolled,
        (ScrollChange{.area = kCode, .line = 1}));
}

TEST(TextAreaBandTest, TextArea_ABandTallerThanThePageStillLeavesARealTopLine)
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

TEST(TextAreaBandTest,
     TextArea_TypingBelowThePaneCountsTheBandsRoom)
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

TEST(TextAreaBandTest, TextArea_TheThumbCountsABandAsContent)
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

TEST(TextAreaBandTest, TextArea_TheThumbSitsAsFarDownAsTheRowsDo)
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

    EXPECT_EQ(
        thumbs[0].rect.origin.y
            + static_cast<std::int32_t>(thumbs[0].rect.size.height),
        static_cast<std::int32_t>(kCanvas.height));
}
