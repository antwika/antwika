#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/UiError.hpp"

using antwika::gfx::Color;
using antwika::gfx::Size;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::FillRect;
using antwika::ui::getFixedSize;
using antwika::ui::Theme;
using antwika::ui::UiError;

namespace
{
    constexpr Color kPanelColor{.red = 10, .green = 20, .blue = 30};

    Theme getPlainTheme(std::uint32_t padding = 0)
    {
        return Theme{
            .panelColor = kPanelColor,
            .textScale = 1,
            .padding = padding,
            .gap = 0,
            .buttonPadding = 0};
    }

    constexpr Size kCanvasSize{.width = 100, .height = 100};
}

TEST(ContainerScopeTest, ContainerScope_ReopensTheParentWhenItEnds)
{
    Context uiContext{kCanvasSize, getPlainTheme()};

    {
        const auto first = uiContext.panel({.heightSizing = getFixedSize(20)});
    }

    {
        const auto second = uiContext.panel({.heightSizing = getFixedSize(20)});
    }

    EXPECT_EQ(
        (DrawList{
            FillRect{
                .rect =
                    {.originPoint = {.x = 0, .y = 0},
                     .size = {.width = 100, .height = 20}},
                .color = kPanelColor},
            FillRect{
                .rect =
                    {.originPoint = {.x = 0, .y = 20},
                     .size = {.width = 100, .height = 20}},
                .color = kPanelColor}}),
        uiContext.build().drawList);
}

TEST(ContainerScopeTest, ContainerScope_PutsWhatFollowsInsideTheOpenContainer)
{
    Context uiContext{kCanvasSize, getPlainTheme(5)};

    {
        const auto outer = uiContext.panel({.heightSizing = getFixedSize(40)});

        {
            const auto inner = uiContext.panel({.heightSizing = getFixedSize(10)});
        }
    }

    EXPECT_EQ(
        (DrawList{
            FillRect{
                .rect =
                    {.originPoint = {.x = 0, .y = 0},
                     .size = {.width = 100, .height = 40}},
                .color = kPanelColor},
            FillRect{
                .rect =
                    {.originPoint = {.x = 5, .y = 5},
                     .size = {.width = 90, .height = 10}},
                .color = kPanelColor}}),
        uiContext.build().drawList);
}

TEST(ContainerScopeTest, Build_RefusesAFrameWithAContainerStillOpen)
{
    Context uiContext{kCanvasSize, getPlainTheme()};

    const auto open = uiContext.panel({.heightSizing = getFixedSize(20)});

    EXPECT_THROW((void)uiContext.build(), UiError);
}

TEST(ContainerScopeTest, Build_RefusesAFrameWithOnlyTheInnerContainerOpen)
{
    Context uiContext{kCanvasSize, getPlainTheme()};

    {
        const auto outer = uiContext.panel({.heightSizing = getFixedSize(20)});
        const auto inner = uiContext.panel({.heightSizing = getFixedSize(10)});

        EXPECT_THROW((void)uiContext.build(), UiError);
    }

    EXPECT_NO_THROW((void)uiContext.build());
}
