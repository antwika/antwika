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
using antwika::ui::getFixedSize;
using antwika::ui::kFitSizing;
using antwika::ui::kGrowSizing;
using antwika::ui::Theme;

namespace
{
    constexpr Color kPanelColor{.red = 10, .green = 20, .blue = 30};
    constexpr Color kAccentColor{.red = 90, .green = 40, .blue = 40};
    constexpr Color kInkColor{.red = 200, .green = 210, .blue = 220};

    Theme getPlainTheme(std::uint32_t padding = 0)
    {
        return Theme{
            .panelColor = kPanelColor,
            .textColor = kInkColor,
            .textScale = 1,
            .padding = padding,
            .gap = 0,
            .buttonPadding = 0};
    }
}

TEST(ContextNestingTest, Nesting_DrawsARowInsideAColumnInsideAPanel)
{
    Context uiContext{Size{.width = 100, .height = 50}, getPlainTheme()};

    {
        const auto outer = uiContext.panel({.heightSizing = kGrowSizing});

        {
            const auto inner = uiContext.row({
                .heightSizing = getFixedSize(10),
                .backgroundColor = kAccentColor});

            uiContext.label("ab");
        }
    }

    EXPECT_EQ(
        (DrawList{
            FillRect{
                .rect =
                    {.originPoint = {.x = 0, .y = 0},
                     .size = {.width = 100, .height = 50}},
                .color = kPanelColor},
            FillRect{
                .rect =
                    {.originPoint = {.x = 0, .y = 0},
                     .size = {.width = 100, .height = 10}},
                .color = kAccentColor},
            DrawText{
                .originPoint = {.x = 0, .y = 0},
                .text = "ab",
                .scale = 1,
                .color = kInkColor}}),
        uiContext.build().drawList);
}

TEST(ContextNestingTest, Nesting_LetsAFittingContainerSizeItself)
{
    Context uiContext{Size{.width = 100, .height = 100}, getPlainTheme(3)};

    {
        const auto box = uiContext.panel(
            {.widthSizing = kFitSizing, .heightSizing = kFitSizing});

        uiContext.label("ab");
    }

    EXPECT_EQ(
        (DrawList{
            FillRect{
                .rect =
                    {.originPoint = {.x = 0, .y = 0},
                     .size = {.width = 18, .height = 14}},
                .color = kPanelColor},
            DrawText{
                .originPoint = {.x = 3, .y = 3},
                .text = "ab",
                .scale = 1,
                .color = kInkColor}}),
        uiContext.build().drawList);
}

TEST(ContextNestingTest, Nesting_PutsSiblingColumnsSideBySide)
{
    Context uiContext{Size{.width = 100, .height = 20}, getPlainTheme()};

    {
        const auto body = uiContext.row({.heightSizing = kGrowSizing});

        {
            const auto side =
                uiContext.column(
                    {.widthSizing = kFitSizing, .heightSizing = kGrowSizing});

            uiContext.label("ab");
        }

        {
            const auto main =
                uiContext.column(
                    {.widthSizing = kGrowSizing, .heightSizing = kGrowSizing});

            uiContext.label("cd");
        }
    }

    EXPECT_EQ(
        (DrawList{
            DrawText{
                .originPoint = {.x = 0, .y = 0},
                .text = "ab",
                .scale = 1,
                .color = kInkColor},
            DrawText{
                .originPoint = {.x = 12, .y = 0},
                .text = "cd",
                .scale = 1,
                .color = kInkColor}}),
        uiContext.build().drawList);
}
