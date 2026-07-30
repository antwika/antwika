#include <gtest/gtest.h>

#include <cstdint>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/ButtonState.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/Theme.hpp"

using antwika::gfx::Color;
using antwika::gfx::Size;
using antwika::ui::ButtonState;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::fixedSize;
using antwika::ui::kGrow;
using antwika::ui::Theme;

namespace
{
    constexpr Color kPanel{.red = 10, .green = 20, .blue = 30};
    constexpr Color kAccent{.red = 90, .green = 40, .blue = 40};
    constexpr Color kInk{.red = 200, .green = 210, .blue = 220};
    constexpr Color kMuted{.red = 100, .green = 110, .blue = 105};
    constexpr Color kIdle{.red = 40, .green = 50, .blue = 60};
    constexpr Color kHovered{.red = 70, .green = 80, .blue = 90};
    constexpr Color kPressed{.red = 15, .green = 25, .blue = 35};
    constexpr Color kButtonInk{.red = 250, .green = 250, .blue = 250};

    Theme plainTheme(std::uint32_t padding = 0)
    {
        return Theme{
            .panel = kPanel,
            .text = kInk,
            .muted = kMuted,
            .buttonIdle = kIdle,
            .buttonHovered = kHovered,
            .buttonPressed = kPressed,
            .buttonText = kButtonInk,
            .textScale = 1,
            .padding = padding,
            .gap = 0,
            .buttonPadding = 0};
    }

    constexpr Size kCanvas{.width = 100, .height = 50};

    [[nodiscard]] Color fillColorOf(const DrawList &commands)
    {
        return std::get<FillRect>(commands.at(0)).color;
    }
} // namespace

TEST(ContextWidgetsTest, Label_UsesTheThemesTextColour)
{
    Context ui{kCanvas, plainTheme()};

    ui.label("ab");

    const auto commands = ui.finish().commands;

    ASSERT_EQ(1U, commands.size());
    EXPECT_EQ(kInk, std::get<DrawText>(commands.at(0)).color);
}

TEST(ContextWidgetsTest, Label_TakesAColourOfItsOwn)
{
    Context ui{kCanvas, plainTheme()};

    ui.label("ab", ui.theme().muted);

    const auto commands = ui.finish().commands;

    ASSERT_EQ(1U, commands.size());
    EXPECT_EQ(kMuted, std::get<DrawText>(commands.at(0)).color);
}

TEST(ContextWidgetsTest, Button_DrawsAFilledBoxAroundItsLabel)
{
    Context ui{kCanvas, plainTheme()};

    ui.button("ab");

    const auto commands = ui.finish().commands;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ(
        (FillRect{
            .rect =
                {.origin = {.x = 0, .y = 0},
                 .size = {.width = 12, .height = 8}},
            .color = kIdle}),
        std::get<FillRect>(commands.at(0)));
    EXPECT_EQ(
        (DrawText{
            .origin = {.x = 0, .y = 0},
            .text = "ab",
            .scale = 1,
            .color = kButtonInk}),
        std::get<DrawText>(commands.at(1)));
}

TEST(ContextWidgetsTest, Button_UsesTheHoveredFillWhenToldTo)
{
    Context ui{kCanvas, plainTheme()};

    ui.button("ab", {.state = ButtonState::Hovered});

    EXPECT_EQ(kHovered, fillColorOf(ui.finish().commands));
}

TEST(ContextWidgetsTest, Button_UsesThePressedFillWhenToldTo)
{
    Context ui{kCanvas, plainTheme()};

    ui.button("ab", {.state = ButtonState::Pressed});

    EXPECT_EQ(kPressed, fillColorOf(ui.finish().commands));
}

// A growing button centres its label out of the room it was given.
TEST(ContextWidgetsTest, Button_CentresItsLabelWhenAskedToGrow)
{
    Context ui{kCanvas, plainTheme()};

    ui.button("ab", {.width = kGrow});

    const auto commands = ui.finish().commands;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ(
        (FillRect{
            .rect =
                {.origin = {.x = 0, .y = 0},
                 .size = {.width = 100, .height = 8}},
            .color = kIdle}),
        std::get<FillRect>(commands.at(0)));

    // A hundred wide, twelve of label: forty-four each side.
    EXPECT_EQ(
        44, std::get<DrawText>(commands.at(1)).origin.x);
}

TEST(ContextWidgetsTest, Spacer_PushesWhatFollowsToTheEndOfARow)
{
    Context ui{Size{.width = 100, .height = 10}, plainTheme()};

    {
        const auto row = ui.row({.height = fixedSize(10)});

        ui.spacer(kGrow);
        ui.label("ab");
    }

    const auto commands = ui.finish().commands;

    ASSERT_EQ(1U, commands.size());
    EXPECT_EQ(88, std::get<DrawText>(commands.at(0)).origin.x);
}

TEST(ContextWidgetsTest, Spacer_PushesWhatFollowsToTheBottomOfAColumn)
{
    Context ui{Size{.width = 100, .height = 100}, plainTheme()};

    {
        const auto column = ui.column({.height = kGrow});

        ui.spacer(kGrow);
        ui.label("ab");
    }

    const auto commands = ui.finish().commands;

    ASSERT_EQ(1U, commands.size());
    EXPECT_EQ(92, std::get<DrawText>(commands.at(0)).origin.y);
}

TEST(ContextWidgetsTest, Panel_FillsInTheThemesBackgroundAndPadding)
{
    Context ui{kCanvas, plainTheme(4)};

    const auto panel = ui.panel({.height = fixedSize(20)});

    ui.label("ab");

    const auto commands = ui.finish().commands;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ(kPanel, fillColorOf(commands));
    EXPECT_EQ(
        (antwika::gfx::Point{.x = 4, .y = 4}),
        std::get<DrawText>(commands.at(1)).origin);
}

TEST(ContextWidgetsTest, Panel_KeepsABackgroundAndPaddingAskedFor)
{
    Context ui{kCanvas, plainTheme(4)};

    const auto panel = ui.panel({
        .height = fixedSize(20),
        .background = kAccent,
        .padding = 0});

    ui.label("ab");

    const auto commands = ui.finish().commands;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ(kAccent, fillColorOf(commands));
    EXPECT_EQ(
        (antwika::gfx::Point{.x = 0, .y = 0}),
        std::get<DrawText>(commands.at(1)).origin);
}
