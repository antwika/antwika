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
using antwika::ui::fixedSize;
using antwika::ui::Theme;
using antwika::ui::UiError;

namespace
{
    constexpr Color kPanel{.red = 10, .green = 20, .blue = 30};

    Theme plainTheme(std::uint32_t padding = 0)
    {
        return Theme{
            .panel = kPanel,
            .textScale = 1,
            .padding = padding,
            .gap = 0,
            .buttonPadding = 0};
    }

    constexpr Size kCanvas{.width = 100, .height = 100};
} // namespace

// The second panel is a sibling of the first, not a child of it.
// That is only true if the first scope closed when it ended.
TEST(ScopeTest, Scope_ReopensTheParentWhenItEnds)
{
    Context ui{kCanvas, plainTheme()};

    {
        const auto first = ui.panel({.height = fixedSize(20)});
    }

    {
        const auto second = ui.panel({.height = fixedSize(20)});
    }

    EXPECT_EQ(
        (DrawList{
            FillRect{
                .rect =
                    {.origin = {.x = 0, .y = 0},
                     .size = {.width = 100, .height = 20}},
                .color = kPanel},
            FillRect{
                .rect =
                    {.origin = {.x = 0, .y = 20},
                     .size = {.width = 100, .height = 20}},
                .color = kPanel}}),
        ui.finish().commands);
}

// A panel opened inside another is inset by the outer one's padding.
// So it really did go in rather than beside.
TEST(ScopeTest, Scope_PutsWhatFollowsInsideTheOpenContainer)
{
    Context ui{kCanvas, plainTheme(5)};

    {
        const auto outer = ui.panel({.height = fixedSize(40)});

        {
            const auto inner = ui.panel({.height = fixedSize(10)});
        }
    }

    EXPECT_EQ(
        (DrawList{
            FillRect{
                .rect =
                    {.origin = {.x = 0, .y = 0},
                     .size = {.width = 100, .height = 40}},
                .color = kPanel},
            FillRect{
                .rect =
                    {.origin = {.x = 5, .y = 5},
                     .size = {.width = 90, .height = 10}},
                .color = kPanel}}),
        ui.finish().commands);
}

// A scope cannot see finish() being called inside it.
// So this is the one thing Context checks rather than forbids.
TEST(ScopeTest, Finish_RefusesAFrameWithAContainerStillOpen)
{
    Context ui{kCanvas, plainTheme()};

    const auto open = ui.panel({.height = fixedSize(20)});

    EXPECT_THROW((void)ui.finish(), UiError);
}

// Nesting makes the check about the container that is still open.
// Whether one was ever opened at all is not what it asks.
TEST(ScopeTest, Finish_RefusesAFrameWithOnlyTheInnerContainerOpen)
{
    Context ui{kCanvas, plainTheme()};

    {
        const auto outer = ui.panel({.height = fixedSize(20)});
        const auto inner = ui.panel({.height = fixedSize(10)});

        EXPECT_THROW((void)ui.finish(), UiError);
    }

    EXPECT_NO_THROW((void)ui.finish());
}
