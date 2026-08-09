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
using antwika::ui::fixedSize;
using antwika::ui::kGrow;
using antwika::ui::Theme;

namespace
{
    constexpr Color kInk{.red = 200, .green = 210, .blue = 220};

    Theme plainTheme(std::uint32_t gap = 0)
    {
        return Theme{
            .text = kInk,
            .textScale = 1,
            .padding = 0,
            .gap = gap,
            .buttonPadding = 0};
    }

    constexpr Size kCanvas{.width = 100, .height = 100};
}

TEST(ContextTest, Finish_GivesNothingForAFrameWithNoWidgets)
{
    Context ui{kCanvas, plainTheme()};

    EXPECT_EQ(DrawList{}, ui.finish().commands);
}

TEST(ContextTest, Theme_IsTheOneItWasGiven)
{
    Context ui{kCanvas, plainTheme()};

    EXPECT_EQ(kInk, ui.theme().text);
    EXPECT_EQ(1U, ui.theme().textScale);
}

TEST(ContextTest, Row_DrawsNothingOfItsOwn)
{
    Context ui{kCanvas, plainTheme()};

    {
        const auto row = ui.row({.height = fixedSize(10)});
    }

    EXPECT_EQ(DrawList{}, ui.finish().commands);
}

TEST(ContextTest, Column_StacksItsChildrenDownwards)
{
    Context ui{kCanvas, plainTheme()};

    ui.label("ab");
    ui.label("cd");

    EXPECT_EQ(
        (DrawList{
            DrawText{
                .origin = {.x = 0, .y = 0},
                .text = "ab",
                .scale = 1,
                .color = kInk},
            DrawText{
                .origin = {.x = 0, .y = 8},
                .text = "cd",
                .scale = 1,
                .color = kInk}}),
        ui.finish().commands);
}

TEST(ContextTest, Context_PutsTheThemeGapBetweenChildren)
{
    Context ui{kCanvas, plainTheme(5)};

    ui.label("ab");
    ui.label("cd");

    const auto commands = ui.finish().commands;

    ASSERT_EQ(2U, commands.size());

    EXPECT_EQ(
        (DrawText{
            .origin = {.x = 0, .y = 13},
            .text = "cd",
            .scale = 1,
            .color = kInk}),
        std::get<DrawText>(commands.at(1)));
}

TEST(ContextTest, Context_PrefersAGapAskedForOverTheThemes)
{
    Context ui{kCanvas, plainTheme(5)};

    {
        const auto column = ui.column({.height = kGrow, .gap = 3});

        ui.label("ab");
        ui.label("cd");
    }

    const auto commands = ui.finish().commands;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ(
        (DrawText{
            .origin = {.x = 0, .y = 11},
            .text = "cd",
            .scale = 1,
            .color = kInk}),
        std::get<DrawText>(commands.at(1)));
}

TEST(ContextTest, Finish_GivesTheSameAnswerTwice)
{
    Context ui{kCanvas, plainTheme()};

    ui.label("ab");

    const auto first = ui.finish().commands;

    EXPECT_EQ(
        (DrawList{DrawText{
            .origin = {.x = 0, .y = 0},
            .text = "ab",
            .scale = 1,
            .color = kInk}}),
        first);
    EXPECT_EQ(first, ui.finish().commands);
}
