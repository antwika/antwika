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
using antwika::ui::kGrowSizing;
using antwika::ui::kSplitRatioScale;
using antwika::ui::Pointer;
using antwika::ui::SplitChange;
using antwika::ui::SplitSpec;
using antwika::ui::Theme;
using antwika::ui::UiError;
using antwika::ui::WidgetId;

namespace
{
    constexpr WidgetId kDividerWidget{1};
    constexpr WidgetId kLeftWidget{2};
    constexpr WidgetId kRightWidget{3};
    constexpr WidgetId kListWidget{4};

    constexpr Size kCanvasSize{.width = 400, .height = 200};

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
        Rect leftRect{};
        Rect dividerRect{};
        Rect rightRect{};
    };

    Panes panesOf(
        const SplitSpec &spec,
        Pointer pointer = {},
        Size canvasSize = kCanvasSize,
        antwika::ui::Interactions *seenInteractions = nullptr)
    {
        Context uiContext(canvasSize, plainTheme(), pointer);

        {
            const auto pair = uiContext.split(spec);

            {
                const auto left = uiContext.column(
                    {.widthSizing = kGrowSizing,
                     .heightSizing = kGrowSizing,
                     .widgetId = kLeftWidget});
            }

            {
                const auto right = uiContext.column(
                    {.widthSizing = kGrowSizing,
                     .heightSizing = kGrowSizing,
                     .widgetId = kRightWidget});
            }
        }

        const auto frame = uiContext.build();

        if (seenInteractions != nullptr)
        {
            *seenInteractions = frame.interactions;
        }

        return Panes{
            .leftRect = frame.rects.find(kLeftWidget).value_or(Rect{}),
            .dividerRect = frame.rects.find(kDividerWidget).value_or(Rect{}),
            .rightRect = frame.rects.find(kRightWidget).value_or(Rect{})};
    }

    [[nodiscard]] SplitSpec evenly()
    {
        return SplitSpec{.widgetId = kDividerWidget, .axis = Axis::Row};
    }

    [[nodiscard]] Pointer pressedAt(std::int32_t x, std::int32_t y)
    {
        return Pointer{
            .positionPoint = Point{.x = x, .y = y},
            .down = true,
            .pressed = true};
    }
}

TEST(SplitTest, Split_HalvesWhatIsLeftOfTheDivider)
{
    const auto panes = panesOf(evenly());

    EXPECT_EQ(panes.dividerRect.size.width, 10U);
    EXPECT_EQ(panes.leftRect.size.width, 195U);
    EXPECT_EQ(panes.rightRect.size.width, 195U);
}

TEST(SplitTest, Split_LaysThePanesEitherSideOfTheDivider)
{
    const auto panes = panesOf(evenly());

    EXPECT_EQ(panes.leftRect.originPoint.x, 0);
    EXPECT_EQ(panes.dividerRect.originPoint.x, 195);
    EXPECT_EQ(panes.rightRect.originPoint.x, 205);
}

TEST(SplitTest, Split_GivesTheFirstPaneTheRatioItAsksFor)
{
    auto spec = evenly();
    spec.ratio = kSplitRatioScale / 4;

    const auto panes = panesOf(spec);

    EXPECT_EQ(panes.leftRect.size.width, 97U);
    EXPECT_EQ(panes.rightRect.size.width, 293U);
}

TEST(SplitTest, Split_GivesTheWholeAxisToTheFirstPaneAtTheTop)
{
    auto spec = evenly();
    spec.ratio = kSplitRatioScale;

    const auto panes = panesOf(spec);

    EXPECT_EQ(panes.leftRect.size.width, 390U);
    EXPECT_EQ(panes.rightRect.size.width, 0U);
}

TEST(SplitTest, Split_ClampsARatioPastTheWhole)
{
    auto spec = evenly();
    spec.ratio = kSplitRatioScale * 3;

    const auto panes = panesOf(spec);

    EXPECT_EQ(panes.leftRect.size.width, 390U);
}

TEST(SplitTest, Split_StopsThePaneAtItsMinimumRatherThanSquashingIt)
{
    auto spec = evenly();
    spec.ratio = 0;
    spec.minimum = 80;

    const auto panes = panesOf(spec);

    EXPECT_EQ(panes.leftRect.size.width, 80U);
    EXPECT_EQ(panes.rightRect.size.width, 310U);
}

TEST(SplitTest, Split_KeepsTheOtherPaneAtItsMinimumToo)
{
    auto spec = evenly();
    spec.ratio = kSplitRatioScale;
    spec.minimum = 80;

    const auto panes = panesOf(spec);

    EXPECT_EQ(panes.leftRect.size.width, 310U);
    EXPECT_EQ(panes.rightRect.size.width, 80U);
}

TEST(SplitTest, Split_SharesEvenlyWhenTwoMinimumsDoNotFit)
{
    auto spec = evenly();
    spec.ratio = 0;
    spec.minimum = 500;

    const auto panes = panesOf(spec);

    EXPECT_EQ(panes.leftRect.size.width, 195U);
    EXPECT_EQ(panes.rightRect.size.width, 195U);
}

TEST(SplitTest, Split_StacksThePanesOnAColumnAxis)
{
    auto spec = evenly();
    spec.axis = Axis::Column;

    const auto panes = panesOf(spec);

    EXPECT_EQ(panes.leftRect.size.height, 95U);
    EXPECT_EQ(panes.dividerRect.originPoint.y, 95);
    EXPECT_EQ(panes.rightRect.originPoint.y, 105);
    EXPECT_EQ(panes.leftRect.size.width, 400U);
}

TEST(SplitTest, Split_LeavesNoContentWhenTheDividerFillsTheAxis)
{
    const auto panes =
        panesOf(evenly(), {}, Size{.width = 6, .height = 200});

    EXPECT_EQ(panes.dividerRect.size.width, 6U);
    EXPECT_EQ(panes.leftRect.size.width, 0U);
    EXPECT_EQ(panes.rightRect.size.width, 0U);
}

TEST(SplitTest, Build_RefusesASplitWithOnePane)
{
    Context uiContext(kCanvasSize, plainTheme());

    {
        const auto pair = uiContext.split(evenly());

        {
            const auto only = uiContext.column({.widgetId = kLeftWidget});
        }
    }

    EXPECT_THROW(static_cast<void>(uiContext.build()), UiError);
}

TEST(SplitTest, Build_RefusesASplitWithThreePanes)
{
    Context uiContext(kCanvasSize, plainTheme());

    {
        const auto pair = uiContext.split(evenly());

        {
            const auto first = uiContext.column({.widgetId = kLeftWidget});
        }

        {
            const auto second = uiContext.column({.widgetId = kRightWidget});
        }

        {
            const auto third = uiContext.column({});
        }
    }

    EXPECT_THROW(static_cast<void>(uiContext.build()), UiError);
}

TEST(SplitTest, Split_ReportsTheRatioAPressOnTheDividerAsksFor)
{
    antwika::ui::Interactions seenInteractions;

    static_cast<void>(
        panesOf(
            evenly(),
            pressedAt(197, 100),
            kCanvasSize,
            &seenInteractions));

    ASSERT_TRUE(seenInteractions.split.has_value());
    EXPECT_EQ(seenInteractions.split->dividerWidget, kDividerWidget);
    EXPECT_EQ(seenInteractions.split->ratio, 492U);
}

TEST(SplitTest, Split_ReportsNothingForAPressBesideTheDivider)
{
    antwika::ui::Interactions seenInteractions;

    static_cast<void>(
        panesOf(
            evenly(),
            pressedAt(20, 100),
            kCanvasSize,
            &seenInteractions));

    EXPECT_FALSE(seenInteractions.split.has_value());
}

TEST(SplitTest, Split_ReportsNothingWhileThePointerIsUp)
{
    antwika::ui::Interactions seenInteractions;

    static_cast<void>(panesOf(
        evenly(),
        Pointer{.positionPoint = Point{.x = 197, .y = 100}},
        kCanvasSize,
        &seenInteractions));

    EXPECT_FALSE(seenInteractions.split.has_value());
}

TEST(SplitTest, Split_KeepsADragThatHasLeftTheDivider)
{
    auto spec = evenly();
    spec.dragging = true;

    antwika::ui::Interactions seenInteractions;

    static_cast<void>(panesOf(
        spec,
        Pointer{
            .positionPoint = Point{.x = 300, .y = 100},
            .down = true,
            .pressed = false},
        kCanvasSize,
        &seenInteractions));

    ASSERT_TRUE(seenInteractions.split.has_value());
    EXPECT_EQ(seenInteractions.split->ratio, 756U);
}

TEST(SplitTest, Split_ReportsTheEndsForADragPastEitherEdge)
{
    auto spec = evenly();
    spec.dragging = true;

    antwika::ui::Interactions beforeInteractions;
    antwika::ui::Interactions afterInteractions;

    const Pointer leftPointer{
        .positionPoint = Point{.x = -50, .y = 100}, .down = true};
    const Pointer rightPointer{
        .positionPoint = Point{.x = 900, .y = 100}, .down = true};

    static_cast<void>(
        panesOf(spec, leftPointer, kCanvasSize, &beforeInteractions));
    static_cast<void>(
        panesOf(spec, rightPointer, kCanvasSize, &afterInteractions));

    ASSERT_TRUE(beforeInteractions.split.has_value());
    ASSERT_TRUE(afterInteractions.split.has_value());
    EXPECT_EQ(beforeInteractions.split->ratio, 0U);
    EXPECT_EQ(afterInteractions.split->ratio, kSplitRatioScale);
}

TEST(SplitTest, Split_ReportsNothingWhenTheDividerFillsTheAxis)
{
    auto spec = evenly();
    spec.dragging = true;

    antwika::ui::Interactions seenInteractions;

    static_cast<void>(panesOf(
        spec,
        Pointer{.positionPoint = Point{.x = 3, .y = 100}, .down = true},
        Size{.width = 6, .height = 200},
        &seenInteractions));

    ASSERT_TRUE(seenInteractions.split.has_value());
    EXPECT_EQ(seenInteractions.split->ratio, 0U);
}

TEST(SplitTest, Split_LeavesTheDividerHoveredUnderThePointer)
{
    antwika::ui::Interactions seenInteractions;

    static_cast<void>(panesOf(
        evenly(),
        Pointer{.positionPoint = Point{.x = 197, .y = 100}},
        kCanvasSize,
        &seenInteractions));

    EXPECT_EQ(seenInteractions.hoveredWidget, kDividerWidget);
    EXPECT_TRUE(seenInteractions.pointerOverUi);
}

TEST(SplitTest, Split_ReportsNothingForADividerWithNoId)
{
    antwika::ui::Interactions seenInteractions;

    auto spec = evenly();
    spec.widgetId = antwika::ui::kNoWidget;
    spec.dragging = true;

    static_cast<void>(panesOf(
        spec, pressedAt(197, 100), kCanvasSize, &seenInteractions));

    EXPECT_FALSE(seenInteractions.split.has_value());
}

TEST(SplitTest, OperatorEquals_ComparesTheDividerAndTheRatio)
{
    constexpr SplitChange baseChange{.dividerWidget = kDividerWidget,
        .ratio = 250};

    EXPECT_EQ(
        baseChange,
        (
            SplitChange{.dividerWidget = kDividerWidget, .ratio = 250}));
    EXPECT_NE(
        baseChange,
        (SplitChange{.dividerWidget = kLeftWidget, .ratio = 250}));
    EXPECT_NE(
        baseChange,
        (SplitChange{.dividerWidget = kDividerWidget, .ratio = 251}));
}

TEST(SplitTest, Split_ReportsTheRatioADragDownAColumnAsksFor)
{
    auto spec = evenly();
    spec.axis = Axis::Column;
    spec.dragging = true;

    antwika::ui::Interactions seenInteractions;

    static_cast<void>(panesOf(
        spec,
        Pointer{.positionPoint = Point{.x = 200, .y = 45}, .down = true},
        kCanvasSize,
        &seenInteractions));

    ASSERT_TRUE(seenInteractions.split.has_value());
    EXPECT_EQ(seenInteractions.split->ratio, 210U);
}

TEST(SplitTest, Split_ReportsNothingForADividerUnderAnOpenList)
{
    constexpr std::array<std::string_view, 2> kOptions{"one", "two"};

    auto spec = evenly();
    spec.dragging = true;

    Context uiContext(
        kCanvasSize,
        plainTheme(),
        Pointer{.positionPoint = Point{.x = 10, .y = 30}, .down = true});

    {
        const auto pair = uiContext.split(spec);

        {
            const auto left = uiContext.column({.widgetId = kLeftWidget});

            uiContext.dropdown(DropdownSpec{
                .widgetId = kListWidget,
                .optionIdBaseWidget = WidgetId{100},
                .options = kOptions,
                .open = true});
        }

        {
            const auto right = uiContext.column({.widgetId = kRightWidget});
        }
    }

    EXPECT_FALSE(uiContext.build().interactions.split.has_value());
}

TEST(SplitTest, Split_IgnoresAHeldPointerThatWanderedOntoTheDivider)
{
    antwika::ui::Interactions seenInteractions;

    static_cast<void>(panesOf(
        evenly(),
        Pointer{
            .positionPoint = Point{.x = 197, .y = 100},
            .down = true,
            .pressed = false},
        kCanvasSize,
        &seenInteractions));

    EXPECT_FALSE(seenInteractions.split.has_value());
}
