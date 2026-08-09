#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string_view>

#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/ContainerSpec.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/DropdownSpec.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/SplitChange.hpp"
#include "antwika/ui/SplitSpec.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/UiError.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::ui::Axis;
using antwika::ui::Context;
using antwika::ui::DropdownSpec;
using antwika::ui::kGrow;
using antwika::ui::kWholeSplit;
using antwika::ui::Pointer;
using antwika::ui::SplitChange;
using antwika::ui::SplitSpec;
using antwika::ui::Theme;
using antwika::ui::UiError;
using antwika::ui::WidgetId;

namespace
{
    constexpr WidgetId kDivider{1};
    constexpr WidgetId kLeft{2};
    constexpr WidgetId kRight{3};
    constexpr WidgetId kList{4};

    constexpr Size kCanvas{.width = 400, .height = 200};

    [[nodiscard]] Theme plainTheme()
    {
        Theme theme;
        theme.padding = 0;
        theme.gap = 0;
        theme.dividerThickness = 10;

        return theme;
    }

    struct Panes final
    {
        Rect left{};
        Rect divider{};
        Rect right{};
    };

    Panes panesOf(
        const SplitSpec &spec,
        Pointer pointer = {},
        Size canvas = kCanvas,
        antwika::ui::Interactions *seen = nullptr)
    {
        Context ui(canvas, plainTheme(), pointer);

        {
            const auto pair = ui.split(spec);

            {
                const auto left = ui.column(
                    {.width = kGrow, .height = kGrow, .id = kLeft});
            }

            {
                const auto right = ui.column(
                    {.width = kGrow, .height = kGrow, .id = kRight});
            }
        }

        const auto frame = ui.finish();

        if (seen != nullptr)
        {
            *seen = frame.interactions;
        }

        return Panes{
            .left = frame.rects.find(kLeft).value_or(Rect{}),
            .divider = frame.rects.find(kDivider).value_or(Rect{}),
            .right = frame.rects.find(kRight).value_or(Rect{})};
    }

    [[nodiscard]] SplitSpec evenly()
    {
        return SplitSpec{.id = kDivider, .axis = Axis::Row};
    }

    [[nodiscard]] Pointer pressedAt(std::int32_t x, std::int32_t y)
    {
        return Pointer{
            .position = Point{.x = x, .y = y},
            .down = true,
            .pressed = true};
    }
}

TEST(SplitTest, Split_HalvesWhatIsLeftOfTheDivider)
{
    const auto panes = panesOf(evenly());

    EXPECT_EQ(panes.divider.size.width, 10U);
    EXPECT_EQ(panes.left.size.width, 195U);
    EXPECT_EQ(panes.right.size.width, 195U);
}

TEST(SplitTest, Split_LaysThePanesEitherSideOfTheDivider)
{
    const auto panes = panesOf(evenly());

    EXPECT_EQ(panes.left.origin.x, 0);
    EXPECT_EQ(panes.divider.origin.x, 195);
    EXPECT_EQ(panes.right.origin.x, 205);
}

TEST(SplitTest, Split_GivesTheFirstPaneTheRatioItAsksFor)
{
    auto spec = evenly();
    spec.ratio = kWholeSplit / 4;

    const auto panes = panesOf(spec);

    EXPECT_EQ(panes.left.size.width, 97U);
    EXPECT_EQ(panes.right.size.width, 293U);
}

TEST(SplitTest, Split_GivesTheWholeAxisToTheFirstPaneAtTheTop)
{
    auto spec = evenly();
    spec.ratio = kWholeSplit;

    const auto panes = panesOf(spec);

    EXPECT_EQ(panes.left.size.width, 390U);
    EXPECT_EQ(panes.right.size.width, 0U);
}

TEST(SplitTest, Split_ClampsARatioPastTheWhole)
{
    auto spec = evenly();
    spec.ratio = kWholeSplit * 3;

    const auto panes = panesOf(spec);

    EXPECT_EQ(panes.left.size.width, 390U);
}

TEST(SplitTest, Split_StopsThePaneAtItsMinimumRatherThanSquashingIt)
{
    auto spec = evenly();
    spec.ratio = 0;
    spec.minimum = 80;

    const auto panes = panesOf(spec);

    EXPECT_EQ(panes.left.size.width, 80U);
    EXPECT_EQ(panes.right.size.width, 310U);
}

TEST(SplitTest, Split_KeepsTheOtherPaneAtItsMinimumToo)
{
    auto spec = evenly();
    spec.ratio = kWholeSplit;
    spec.minimum = 80;

    const auto panes = panesOf(spec);

    EXPECT_EQ(panes.left.size.width, 310U);
    EXPECT_EQ(panes.right.size.width, 80U);
}

TEST(SplitTest, Split_SharesEvenlyWhenTwoMinimumsDoNotFit)
{
    auto spec = evenly();
    spec.ratio = 0;
    spec.minimum = 500;

    const auto panes = panesOf(spec);

    EXPECT_EQ(panes.left.size.width, 195U);
    EXPECT_EQ(panes.right.size.width, 195U);
}

TEST(SplitTest, Split_StacksThePanesOnAColumnAxis)
{
    auto spec = evenly();
    spec.axis = Axis::Column;

    const auto panes = panesOf(spec);

    EXPECT_EQ(panes.left.size.height, 95U);
    EXPECT_EQ(panes.divider.origin.y, 95);
    EXPECT_EQ(panes.right.origin.y, 105);
    EXPECT_EQ(panes.left.size.width, 400U);
}

TEST(SplitTest, Split_LeavesNoContentWhenTheDividerFillsTheAxis)
{
    const auto panes =
        panesOf(evenly(), {}, Size{.width = 6, .height = 200});

    EXPECT_EQ(panes.divider.size.width, 6U);
    EXPECT_EQ(panes.left.size.width, 0U);
    EXPECT_EQ(panes.right.size.width, 0U);
}

TEST(SplitTest, Finish_RefusesASplitWithOnePane)
{
    Context ui(kCanvas, plainTheme());

    {
        const auto pair = ui.split(evenly());

        {
            const auto only = ui.column({.id = kLeft});
        }
    }

    EXPECT_THROW(static_cast<void>(ui.finish()), UiError);
}

TEST(SplitTest, Finish_RefusesASplitWithThreePanes)
{
    Context ui(kCanvas, plainTheme());

    {
        const auto pair = ui.split(evenly());

        {
            const auto first = ui.column({.id = kLeft});
        }

        {
            const auto second = ui.column({.id = kRight});
        }

        {
            const auto third = ui.column({});
        }
    }

    EXPECT_THROW(static_cast<void>(ui.finish()), UiError);
}

TEST(SplitTest, Split_ReportsTheRatioAPressOnTheDividerAsksFor)
{
    antwika::ui::Interactions seen;

    static_cast<void>(panesOf(evenly(), pressedAt(197, 100), kCanvas, &seen));

    ASSERT_TRUE(seen.split.has_value());
    EXPECT_EQ(seen.split->divider, kDivider);
    EXPECT_EQ(seen.split->ratio, 492U);
}

TEST(SplitTest, Split_ReportsNothingForAPressBesideTheDivider)
{
    antwika::ui::Interactions seen;

    static_cast<void>(panesOf(evenly(), pressedAt(20, 100), kCanvas, &seen));

    EXPECT_FALSE(seen.split.has_value());
}

TEST(SplitTest, Split_ReportsNothingWhileThePointerIsUp)
{
    antwika::ui::Interactions seen;

    static_cast<void>(panesOf(
        evenly(),
        Pointer{.position = Point{.x = 197, .y = 100}},
        kCanvas,
        &seen));

    EXPECT_FALSE(seen.split.has_value());
}

TEST(SplitTest, Split_KeepsADragThatHasLeftTheDivider)
{
    auto spec = evenly();
    spec.dragging = true;

    antwika::ui::Interactions seen;

    static_cast<void>(panesOf(
        spec,
        Pointer{
            .position = Point{.x = 300, .y = 100},
            .down = true,
            .pressed = false},
        kCanvas,
        &seen));

    ASSERT_TRUE(seen.split.has_value());
    EXPECT_EQ(seen.split->ratio, 756U);
}

TEST(SplitTest, Split_ReportsTheEndsForADragPastEitherEdge)
{
    auto spec = evenly();
    spec.dragging = true;

    antwika::ui::Interactions before;
    antwika::ui::Interactions after;

    const Pointer left{
        .position = Point{.x = -50, .y = 100}, .down = true};
    const Pointer right{
        .position = Point{.x = 900, .y = 100}, .down = true};

    static_cast<void>(panesOf(spec, left, kCanvas, &before));
    static_cast<void>(panesOf(spec, right, kCanvas, &after));

    ASSERT_TRUE(before.split.has_value());
    ASSERT_TRUE(after.split.has_value());
    EXPECT_EQ(before.split->ratio, 0U);
    EXPECT_EQ(after.split->ratio, kWholeSplit);
}

TEST(SplitTest, Split_ReportsNothingWhenTheDividerFillsTheAxis)
{
    auto spec = evenly();
    spec.dragging = true;

    antwika::ui::Interactions seen;

    static_cast<void>(panesOf(
        spec,
        Pointer{.position = Point{.x = 3, .y = 100}, .down = true},
        Size{.width = 6, .height = 200},
        &seen));

    ASSERT_TRUE(seen.split.has_value());
    EXPECT_EQ(seen.split->ratio, 0U);
}

TEST(SplitTest, Split_LeavesTheDividerHoveredUnderThePointer)
{
    antwika::ui::Interactions seen;

    static_cast<void>(panesOf(
        evenly(),
        Pointer{.position = Point{.x = 197, .y = 100}},
        kCanvas,
        &seen));

    EXPECT_EQ(seen.hovered, kDivider);
    EXPECT_TRUE(seen.pointerOverUi);
}

TEST(SplitTest, Split_ReportsNothingForADividerWithNoId)
{
    antwika::ui::Interactions seen;

    auto spec = evenly();
    spec.id = antwika::ui::kNoWidget;
    spec.dragging = true;

    static_cast<void>(panesOf(spec, pressedAt(197, 100), kCanvas, &seen));

    EXPECT_FALSE(seen.split.has_value());
}

TEST(SplitTest, OperatorEquals_ComparesTheDividerAndTheRatio)
{
    constexpr SplitChange base{.divider = kDivider, .ratio = 250};

    EXPECT_EQ(base, (SplitChange{.divider = kDivider, .ratio = 250}));
    EXPECT_NE(base, (SplitChange{.divider = kLeft, .ratio = 250}));
    EXPECT_NE(base, (SplitChange{.divider = kDivider, .ratio = 251}));
}

TEST(SplitTest, Split_ReportsTheRatioADragDownAColumnAsksFor)
{
    auto spec = evenly();
    spec.axis = Axis::Column;
    spec.dragging = true;

    antwika::ui::Interactions seen;

    static_cast<void>(panesOf(
        spec,
        Pointer{.position = Point{.x = 200, .y = 45}, .down = true},
        kCanvas,
        &seen));

    ASSERT_TRUE(seen.split.has_value());
    EXPECT_EQ(seen.split->ratio, 210U);
}

TEST(SplitTest, Split_ReportsNothingForADividerUnderAnOpenList)
{
    constexpr std::array<std::string_view, 2> kOptions{"one", "two"};

    auto spec = evenly();
    spec.dragging = true;

    Context ui(
        kCanvas,
        plainTheme(),
        Pointer{.position = Point{.x = 10, .y = 30}, .down = true});

    {
        const auto pair = ui.split(spec);

        {
            const auto left = ui.column({.id = kLeft});

            ui.dropdown(DropdownSpec{
                .id = kList,
                .optionIdBase = WidgetId{100},
                .options = kOptions,
                .open = true});
        }

        {
            const auto right = ui.column({.id = kRight});
        }
    }

    EXPECT_FALSE(ui.finish().interactions.split.has_value());
}

TEST(SplitTest, Split_IgnoresAHeldPointerThatWanderedOntoTheDivider)
{
    antwika::ui::Interactions seen;

    static_cast<void>(panesOf(
        evenly(),
        Pointer{
            .position = Point{.x = 197, .y = 100},
            .down = true,
            .pressed = false},
        kCanvas,
        &seen));

    EXPECT_FALSE(seen.split.has_value());
}
