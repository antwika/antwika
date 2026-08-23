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
using antwika::widget::kNoWidget;
using antwika::ui::LineRun;
using antwika::ui::Pointer;
using antwika::ui::ScrollChange;
using antwika::ui::TextAreaSpec;
using antwika::ui::Theme;
using antwika::widget::WidgetId;
using antwika::ui::tests::linesOf;

namespace
{
    constexpr Color kInkColor{.red = 200, .green = 210, .blue = 220};
    constexpr Color kThumbColor{.red = 78, .green = 86, .blue = 106};

    constexpr WidgetId kCodeWidget{9};
    constexpr WidgetId kBandWidget{40};
    constexpr WidgetId kOtherWidget{41};

    constexpr Size kCanvasSize{.width = 200, .height = 64};
    constexpr std::uint32_t kLineHeight = 8;

    Theme getPlainTheme()
    {
        return Theme{
            .textColor = kInkColor,
            .scrollThumbColor = kThumbColor,
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
        spec.widgetId = kCodeWidget;
        spec.focused = true;

        Context uiContext{kCanvasSize, getPlainTheme(), pointer, keyboard};

        uiContext.textArea(spec);

        return uiContext.build();
    }

    [[nodiscard]] std::int32_t topOf(
        const Frame &frame, const std::string &text)
    {
        for (const auto &command : frame.drawList)
        {
            const auto *drawText = std::get_if<DrawText>(&command);

            if (drawText != nullptr && drawText->text == text)
            {
                return drawText->originPoint.y;
            }
        }

        return -1;
    }

    [[nodiscard]] std::vector<FillRect> fillsOf(
        const Frame &frame, Color color)
    {
        std::vector<FillRect> fills;

        for (const auto &command : frame.drawList)
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
    const std::vector<LineRun> bandRuns{
        LineRun{.line = 1, .rows = 2, .widgetId = kBandWidget}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bandRuns = bandRuns});

    const auto foundRect = frame.rects.getFind(kBandWidget);

    ASSERT_TRUE(foundRect.has_value());
    EXPECT_EQ(
        *foundRect,
        (Rect{
            .originPoint = {.x = 0, .y = 2 * kLineHeight},
            .size = {.width = kCanvasSize.width, .height = 2 * kLineHeight}}));

    EXPECT_EQ(topOf(frame, "c"), 4 * kLineHeight);
}

TEST(TextAreaBandTest, TextArea_ABandOfNoRowsHoldsNothing)
{
    const std::vector<LineRun> bandRuns{
        LineRun{.line = 1, .rows = 0, .widgetId = kBandWidget}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bandRuns = bandRuns});

    EXPECT_FALSE(frame.rects.getFind(kBandWidget).has_value());
    EXPECT_EQ(topOf(frame, "c"), 2 * kLineHeight);
}

TEST(TextAreaBandTest, TextArea_ABandNamingALineTheTextDoesNotHaveHoldsNothing)
{
    const std::vector<LineRun> bandRuns{
        LineRun{.line = 50, .rows = 2, .widgetId = kBandWidget}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bandRuns = bandRuns});

    EXPECT_FALSE(frame.rects.getFind(kBandWidget).has_value());
}

TEST(TextAreaBandTest, TextArea_TwoBandsOnOneLineStackInDeclarationOrder)
{
    const std::vector<LineRun> bandRuns{
        LineRun{.line = 0, .rows = 1, .widgetId = kBandWidget},
        LineRun{.line = 0, .rows = 1, .widgetId = kOtherWidget}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bandRuns = bandRuns});

    const auto first = frame.rects.getFind(kBandWidget);
    const auto second = frame.rects.getFind(kOtherWidget);

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->originPoint.y, 1 * kLineHeight);
    EXPECT_EQ(second->originPoint.y, 2 * kLineHeight);

    EXPECT_NE(bandRuns[0], bandRuns[1]);
    EXPECT_NE(
        bandRuns[0],
        (
            LineRun{.line = 1, .rows = 1, .widgetId = kBandWidget}));
    EXPECT_NE(
        bandRuns[0],
        (LineRun{.line = 0, .rows = 2, .widgetId = kBandWidget}));
    EXPECT_EQ(
        bandRuns[0],
        (LineRun{.line = 0, .rows = 1, .widgetId = kBandWidget}));
}

TEST(TextAreaBandTest, TextArea_ABandScrolledOffTheTopIsOffWithIt)
{
    const std::vector<LineRun> bandRuns{
        LineRun{.line = 1, .rows = 2, .widgetId = kBandWidget}};

    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(20),
            .cursor = 0,
            .scroll = 2,
            .bandRuns = bandRuns});

    EXPECT_FALSE(frame.rects.getFind(kBandWidget).has_value());
}

TEST(TextAreaBandTest, TextArea_AnUnnamedBandStillHoldsItsRoom)
{
    const std::vector<LineRun> bandRuns{
        LineRun{.line = 0, .rows = 1, .widgetId = kNoWidget}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bandRuns = bandRuns});

    EXPECT_EQ(topOf(frame, "b"), 2 * kLineHeight);
}

TEST(TextAreaBandTest, TextArea_AClickBelowABandLandsOnTheLineThatWasUnderIt)
{
    const std::vector<LineRun> bandRuns{
        LineRun{.line = 0, .rows = 2, .widgetId = kBandWidget}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bandRuns = bandRuns},
        Keyboard{},
        Pointer{
            .positionPoint = Point{.x = 0, .y = 3 * kLineHeight + 1},
            .down = true,
            .pressed = true});

    ASSERT_TRUE(frame.interactions.edit.has_value());

    EXPECT_EQ(frame.interactions.edit->cursor, 2U);
}

TEST(TextAreaBandTest, TextArea_AClickInsideABandBelongsToTheLineItHangsUnder)
{
    const std::vector<LineRun> bandRuns{
        LineRun{.line = 0, .rows = 2, .widgetId = kBandWidget}};

    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(4), .cursor = 0, .bandRuns = bandRuns},
        Keyboard{},
        Pointer{
            .positionPoint = Point{.x = 40, .y = kLineHeight + 1},
            .down = true,
            .pressed = true});

    ASSERT_TRUE(frame.interactions.edit.has_value());

    EXPECT_EQ(frame.interactions.edit->cursor, 1U);
}

TEST(TextAreaBandTest, TextArea_AClickBelowEveryLineIsTheEndOfTheDocument)
{
    const auto text = linesOf(4);

    const std::vector<LineRun> bandRuns{
        LineRun{.line = 0, .rows = 2, .widgetId = kBandWidget}};

    const auto frame = frameOf(
        TextAreaSpec{.text = text, .cursor = 0, .bandRuns = bandRuns},
        Keyboard{},
        Pointer{
            .positionPoint = Point{.x = 0, .y = 7 * kLineHeight + 1},
            .down = true,
            .pressed = true});

    ASSERT_TRUE(frame.interactions.edit.has_value());
    EXPECT_EQ(frame.interactions.edit->cursor, text.size());
}

TEST(TextAreaBandTest, TextArea_AskingForALineTooFarDownCountsTheBandsRoom)
{
    const std::vector<LineRun> bandRuns{
        LineRun{.line = 3, .rows = 4, .widgetId = kBandWidget}};

    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(6),
            .cursor = 0,
            .scroll = 500,
            .bandRuns = bandRuns});

    EXPECT_EQ(
        frame.interactions.scrollChange,
        (ScrollChange{.areaWidget = kCodeWidget, .line = 2}));
}

TEST(TextAreaBandTest, TextArea_ABandDecidesHowFarDownThePaneCanGo)
{
    const std::vector<LineRun> bandRuns{
        LineRun{.line = 0, .rows = 6, .widgetId = kBandWidget}};

    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(4),
            .cursor = 0,
            .scroll = 500,
            .bandRuns = bandRuns});

    EXPECT_EQ(
        frame.interactions.scrollChange,
        (ScrollChange{.areaWidget = kCodeWidget, .line = 1}));
}

TEST(TextAreaBandTest, TextArea_ABandTallerThanThePageStillLeavesARealTopLine)
{
    const std::vector<LineRun> bandRuns{
        LineRun{.line = 1, .rows = 20, .widgetId = kBandWidget}};

    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(2),
            .cursor = 0,
            .scroll = 500,
            .bandRuns = bandRuns});

    EXPECT_EQ(
        frame.interactions.scrollChange,
        (ScrollChange{.areaWidget = kCodeWidget, .line = 1}));
}

TEST(TextAreaBandTest,
     TextArea_TypingBelowThePaneCountsTheBandsRoom)
{
    const auto text = linesOf(20);

    const std::vector<LineRun> bandRuns{
        LineRun{.line = 16, .rows = 4, .widgetId = kBandWidget}};

    const auto frame = frameOf(
        TextAreaSpec{
            .text = text,
            .cursor = text.size(),
            .scroll = 0,
            .bandRuns = bandRuns},
        Keyboard{.keys = {Key::Character}, .typedText = "X"});

    ASSERT_TRUE(frame.interactions.scrollChange.has_value());
    EXPECT_EQ(frame.interactions.scrollChange->line, 16U);
}

TEST(TextAreaBandTest, TextArea_TheThumbCountsABandAsContent)
{
    const std::vector<LineRun> bandRuns{
        LineRun{.line = 0, .rows = 16, .widgetId = kBandWidget}};

    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(16),
            .cursor = 0,
            .bandRuns = bandRuns,
            .scrollbar = true});

    const auto thumbs = fillsOf(frame, kThumbColor);

    ASSERT_EQ(thumbs.size(), 1U);
    EXPECT_EQ(thumbs[0].rect.size.height, kCanvasSize.height / 4);
}

TEST(TextAreaBandTest, TextArea_TheThumbSitsAsFarDownAsTheRowsDo)
{
    const std::vector<LineRun> bandRuns{
        LineRun{.line = 0, .rows = 16, .widgetId = kBandWidget}};

    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(16),
            .cursor = 0,
            .scroll = 500,
            .bandRuns = bandRuns,
            .scrollbar = true});

    const auto thumbs = fillsOf(frame, kThumbColor);

    ASSERT_EQ(thumbs.size(), 1U);

    EXPECT_EQ(
        thumbs[0].rect.originPoint.y
            + static_cast<std::int32_t>(thumbs[0].rect.size.height),
        static_cast<std::int32_t>(kCanvasSize.height));
}
