#include <gtest/gtest.h>

#include <cstdint>

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
using antwika::ui::FillRect;
using antwika::ui::fixedSize;
using antwika::ui::kFit;
using antwika::ui::kGrow;
using antwika::ui::Theme;

namespace
{
    constexpr Color kPanel{.red = 10, .green = 20, .blue = 30};
    constexpr Color kAccent{.red = 90, .green = 40, .blue = 40};
    constexpr Color kInk{.red = 200, .green = 210, .blue = 220};

    Theme plainTheme(std::uint32_t padding = 0)
    {
        return Theme{
            .panel = kPanel,
            .text = kInk,
            .textScale = 1,
            .padding = padding,
            .gap = 0,
            .buttonPadding = 0};
    }
}

TEST(ContextNestingTest, Nesting_DrawsARowInsideAColumnInsideAPanel)
{
    Context ui{Size{.width = 100, .height = 50}, plainTheme()};

    {
        const auto outer = ui.panel({.height = kGrow});

        {
            const auto inner = ui.row({
                .height = fixedSize(10),
                .background = kAccent});

            ui.label("ab");
        }
    }

    EXPECT_EQ(
        (DrawList{
            FillRect{
                .rect =
                    {.origin = {.x = 0, .y = 0},
                     .size = {.width = 100, .height = 50}},
                .color = kPanel},
            FillRect{
                .rect =
                    {.origin = {.x = 0, .y = 0},
                     .size = {.width = 100, .height = 10}},
                .color = kAccent},
            DrawText{
                .origin = {.x = 0, .y = 0},
                .text = "ab",
                .scale = 1,
                .color = kInk}}),
        ui.finish().commands);
}

TEST(ContextNestingTest, Nesting_LetsAFittingContainerSizeItself)
{
    Context ui{Size{.width = 100, .height = 100}, plainTheme(3)};

    {
        const auto box = ui.panel({.width = kFit, .height = kFit});

        ui.label("ab");
    }

    EXPECT_EQ(
        (DrawList{
            FillRect{
                .rect =
                    {.origin = {.x = 0, .y = 0},
                     .size = {.width = 18, .height = 14}},
                .color = kPanel},
            DrawText{
                .origin = {.x = 3, .y = 3},
                .text = "ab",
                .scale = 1,
                .color = kInk}}),
        ui.finish().commands);
}

TEST(ContextNestingTest, Nesting_PutsSiblingColumnsSideBySide)
{
    Context ui{Size{.width = 100, .height = 20}, plainTheme()};

    {
        const auto body = ui.row({.height = kGrow});

        {
            const auto side = ui.column({.width = kFit, .height = kGrow});

            ui.label("ab");
        }

        {
            const auto main = ui.column({.width = kGrow, .height = kGrow});

            ui.label("cd");
        }
    }

    EXPECT_EQ(
        (DrawList{
            DrawText{
                .origin = {.x = 0, .y = 0},
                .text = "ab",
                .scale = 1,
                .color = kInk},
            DrawText{
                .origin = {.x = 12, .y = 0},
                .text = "cd",
                .scale = 1,
                .color = kInk}}),
        ui.finish().commands);
}
