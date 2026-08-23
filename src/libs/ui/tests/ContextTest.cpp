#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/Theme.hpp"

using antwika::gfx::Color;
using antwika::gfx::Size;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::getFixedSize;
using antwika::ui::kGrowSizing;
using antwika::ui::Theme;

namespace
{
    constexpr Color kInkColor{.red = 200, .green = 210, .blue = 220};

    Theme getPlainTheme(std::uint32_t gap = 0)
    {
        return Theme{
            .textColor = kInkColor,
            .textScale = 1,
            .padding = 0,
            .gap = gap,
            .buttonPadding = 0};
    }

    constexpr Size kCanvasSize{.width = 100, .height = 100};
}

TEST(ContextTest, Build_GivesNothingForAFrameWithNoWidgets)
{
    Context uiContext{kCanvasSize, getPlainTheme()};

    EXPECT_EQ(DrawList{}, uiContext.build().drawList);
}

TEST(ContextTest, Theme_IsTheOneItWasGiven)
{
    Context uiContext{kCanvasSize, getPlainTheme()};

    EXPECT_EQ(kInkColor, uiContext.getTheme().textColor);
    EXPECT_EQ(1U, uiContext.getTheme().textScale);
}

TEST(ContextTest, Row_DrawsNothingOfItsOwn)
{
    Context uiContext{kCanvasSize, getPlainTheme()};

    {
        const auto row = uiContext.row({.heightSizing = getFixedSize(10)});
    }

    EXPECT_EQ(DrawList{}, uiContext.build().drawList);
}

TEST(ContextTest, Column_StacksItsChildrenDownwards)
{
    Context uiContext{kCanvasSize, getPlainTheme()};

    uiContext.label("ab");
    uiContext.label("cd");

    EXPECT_EQ(
        (DrawList{
            DrawText{
                .originPoint = {.x = 0, .y = 0},
                .text = "ab",
                .scale = 1,
                .color = kInkColor},
            DrawText{
                .originPoint = {.x = 0, .y = 8},
                .text = "cd",
                .scale = 1,
                .color = kInkColor}}),
        uiContext.build().drawList);
}

TEST(ContextTest, Context_PutsTheThemeGapBetweenChildren)
{
    Context uiContext{kCanvasSize, getPlainTheme(5)};

    uiContext.label("ab");
    uiContext.label("cd");

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(2U, commands.size());

    EXPECT_EQ(
        (DrawText{
            .originPoint = {.x = 0, .y = 13},
            .text = "cd",
            .scale = 1,
            .color = kInkColor}),
        std::get<DrawText>(commands.at(1)));
}

TEST(ContextTest, Context_PrefersAGapAskedForOverTheThemes)
{
    Context uiContext{kCanvasSize, getPlainTheme(5)};

    {
        const auto column = uiContext.column(
            {.heightSizing = kGrowSizing, .gap = 3});

        uiContext.label("ab");
        uiContext.label("cd");
    }

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ(
        (DrawText{
            .originPoint = {.x = 0, .y = 11},
            .text = "cd",
            .scale = 1,
            .color = kInkColor}),
        std::get<DrawText>(commands.at(1)));
}

TEST(ContextTest, Build_GivesTheSameAnswerTwice)
{
    Context uiContext{kCanvasSize, getPlainTheme()};

    uiContext.label("ab");

    const auto first = uiContext.build().drawList;

    EXPECT_EQ(
        (DrawList{DrawText{
            .originPoint = {.x = 0, .y = 0},
            .text = "ab",
            .scale = 1,
            .color = kInkColor}}),
        first);
    EXPECT_EQ(first, uiContext.build().drawList);
}
