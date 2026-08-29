#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include <antwika/ui/support/DrawListQueries.hpp>

#include "antwika/ui/ContainerSpec.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/DropdownSpec.hpp"
#include "antwika/ui/EdgeChange.hpp"
#include "antwika/ui/EdgeSpec.hpp"
#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::ui::Axis;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::DropdownSpec;
using antwika::ui::EdgeChange;
using antwika::ui::EdgeSpec;
using antwika::ui::getFixedSize;
using antwika::ui::Interactions;
using antwika::ui::kGrowSizing;
using antwika::ui::Pointer;
using antwika::ui::Theme;
using antwika::widget::WidgetId;

namespace
{
    constexpr WidgetId kEdgeWidget{1};
    constexpr WidgetId kPanelWidget{2};
    constexpr WidgetId kRestWidget{3};
    constexpr WidgetId kListWidget{4};

    constexpr Size kCanvasSize{.width = 400, .height = 200};

    constexpr std::uint32_t kThickness = 10;

    constexpr std::uint32_t kPanelSide = 100;

    constexpr antwika::gfx::Color kIdleColor{
        .red = 10, .green = 20, .blue = 30, .alpha = 255};

    constexpr antwika::gfx::Color kHoveredColor{
        .red = 40, .green = 50, .blue = 60, .alpha = 255};

    constexpr antwika::gfx::Color kHeldColor{
        .red = 70, .green = 80, .blue = 90, .alpha = 255};

    [[nodiscard]] Theme getPlainTheme()
    {
        Theme theme;
        theme.padding = 0;
        theme.gap = 0;
        theme.dividerThickness = kThickness;

        return theme;
    }

    struct Bands final
    {
        Rect panelRect{};
        Rect edgeRect{};
        Rect restRect{};
        Interactions seenInteractions{};
        DrawList drawList{};
    };

    [[nodiscard]] Theme getLitTheme()
    {
        auto theme = getPlainTheme();
        theme.dividerColor = kIdleColor;
        theme.dividerHoveredColor = kHoveredColor;
        theme.dividerHeldColor = kHeldColor;

        return theme;
    }

    [[nodiscard]] EdgeSpec getPlainEdge()
    {
        return EdgeSpec{
            .widgetId = kEdgeWidget, .panelWidget = kPanelWidget};
    }

    [[nodiscard]] Bands bandsOf(
        const EdgeSpec &spec,
        const Pointer pointer = {},
        const Axis axis = Axis::Row,
        const bool panelFirst = true)
    {
        Context uiContext(kCanvasSize, getLitTheme(), pointer);

        {
            const auto outer =
                axis == Axis::Row
                    ? uiContext.row(
                          {.widthSizing = kGrowSizing,
                           .heightSizing = kGrowSizing})
                    : uiContext.column(
                          {.widthSizing = kGrowSizing,
                           .heightSizing = kGrowSizing});

            const auto panelSizing = getFixedSize(kPanelSide);

            const auto dropPanel = [&]
            {
                const auto panel = uiContext.column(
                    {.widthSizing =
                         axis == Axis::Row ? panelSizing : kGrowSizing,
                     .heightSizing =
                         axis == Axis::Row ? kGrowSizing : panelSizing,
                     .widgetId = kPanelWidget});
            };

            const auto dropRest = [&]
            {
                const auto rest = uiContext.column(
                    {.widthSizing = kGrowSizing,
                     .heightSizing = kGrowSizing,
                     .widgetId = kRestWidget});
            };

            if (panelFirst)
            {
                dropPanel();
                uiContext.edge(spec);
                dropRest();
            }
            else
            {
                dropRest();
                uiContext.edge(spec);
                dropPanel();
            }
        }

        const auto frame = uiContext.build();

        return Bands{
            .panelRect =
                frame.rects.getWidgetRect(kPanelWidget).value_or(Rect{}),
            .edgeRect =
                frame.rects.getWidgetRect(kEdgeWidget).value_or(Rect{}),
            .restRect =
                frame.rects.getWidgetRect(kRestWidget).value_or(Rect{}),
            .seenInteractions = frame.interactions,
            .drawList = frame.drawList};
    }

    [[nodiscard]] Pointer pressedAt(
        const std::int32_t x, const std::int32_t y)
    {
        return Pointer{
            .positionPoint = Point{.x = x, .y = y},
            .down = true,
            .pressed = true};
    }

    [[nodiscard]] Pointer heldAt(
        const std::int32_t x, const std::int32_t y)
    {
        return Pointer{
            .positionPoint = Point{.x = x, .y = y},
            .down = true,
            .pressed = false};
    }
}

TEST(EdgeTest, Edge_TakesTheDividerThicknessOnTheRowAxis)
{
    const auto bands = bandsOf(getPlainEdge());

    EXPECT_EQ(bands.edgeRect.size.width, kThickness);
    EXPECT_EQ(bands.edgeRect.size.height, kCanvasSize.height);
    EXPECT_EQ(bands.edgeRect.originPoint.x, kPanelSide);
}

TEST(EdgeTest, Edge_TakesTheDividerThicknessOnTheColumnAxis)
{
    const auto bands = bandsOf(getPlainEdge(), {}, Axis::Column);

    EXPECT_EQ(bands.edgeRect.size.height, kThickness);
    EXPECT_EQ(bands.edgeRect.size.width, kCanvasSize.width);
    EXPECT_EQ(bands.edgeRect.originPoint.y, kPanelSide);
}

TEST(EdgeTest, Edge_LeavesTheBandsEitherSideOfIt)
{
    const auto bands = bandsOf(getPlainEdge());

    EXPECT_EQ(bands.panelRect.size.width, kPanelSide);
    EXPECT_EQ(bands.restRect.originPoint.x, kPanelSide + kThickness);
    EXPECT_EQ(
        bands.restRect.size.width, kCanvasSize.width - kPanelSide - kThickness);
}

TEST(EdgeTest, Edge_ReportsTheExtentAPressAsksFor)
{
    const auto bands = bandsOf(getPlainEdge(), pressedAt(104, 100));

    ASSERT_TRUE(bands.seenInteractions.edge.has_value());
    EXPECT_EQ(bands.seenInteractions.edge->edgeWidget, kEdgeWidget);
    EXPECT_EQ(bands.seenInteractions.edge->extent, 99U);
}

TEST(EdgeTest, Edge_ReportsTheExtentForAPanelAfterTheBar)
{
    auto spec = getPlainEdge();
    spec.dragging = true;

    const auto bands =
        bandsOf(spec, heldAt(120, 100), Axis::Row, false);

    ASSERT_TRUE(bands.seenInteractions.edge.has_value());
    EXPECT_EQ(bands.seenInteractions.edge->extent, 275U);
}

TEST(EdgeTest, Edge_ReportsTheExtentADragDownAColumnAsksFor)
{
    auto spec = getPlainEdge();
    spec.dragging = true;

    const auto bands = bandsOf(spec, heldAt(200, 60), Axis::Column);

    ASSERT_TRUE(bands.seenInteractions.edge.has_value());
    EXPECT_EQ(bands.seenInteractions.edge->extent, 55U);
}

TEST(EdgeTest, Edge_ReportsNothingForAPressBesideTheBar)
{
    const auto bands = bandsOf(getPlainEdge(), pressedAt(20, 100));

    EXPECT_FALSE(bands.seenInteractions.edge.has_value());
}

TEST(EdgeTest, Edge_KeepsTheDragOnceItHasBegun)
{
    auto spec = getPlainEdge();
    spec.dragging = true;

    const auto bands = bandsOf(spec, heldAt(300, 100));

    ASSERT_TRUE(bands.seenInteractions.edge.has_value());
    EXPECT_EQ(bands.seenInteractions.edge->extent, 295U);
}

TEST(EdgeTest, Edge_TakesNoHoldWithoutAPressOfItsOwn)
{
    for (const auto pointer :
         {Pointer{.positionPoint = Point{.x = 104, .y = 100}},
          heldAt(104, 100)})
    {
        const auto bands = bandsOf(getPlainEdge(), pointer);

        EXPECT_FALSE(bands.seenInteractions.edge.has_value());
    }
}

TEST(EdgeTest, Edge_HoldsTheExtentAtTheMinimum)
{
    auto spec = getPlainEdge();
    spec.dragging = true;
    spec.minimum = 60;

    const auto bands = bandsOf(spec, heldAt(-40, 100));

    ASSERT_TRUE(bands.seenInteractions.edge.has_value());
    EXPECT_EQ(bands.seenInteractions.edge->extent, 60U);
}

TEST(EdgeTest, Edge_HoldsTheExtentAtTheMaximum)
{
    auto spec = getPlainEdge();
    spec.dragging = true;
    spec.maximum = 150;

    const auto bands = bandsOf(spec, heldAt(900, 100));

    ASSERT_TRUE(bands.seenInteractions.edge.has_value());
    EXPECT_EQ(bands.seenInteractions.edge->extent, 150U);
}

TEST(EdgeTest, Edge_HoldsAMaximumUnderTheMinimumAtTheMinimum)
{
    auto spec = getPlainEdge();
    spec.dragging = true;
    spec.minimum = 200;
    spec.maximum = 150;

    const auto bands = bandsOf(spec, heldAt(0, 100));

    ASSERT_TRUE(bands.seenInteractions.edge.has_value());
    EXPECT_EQ(bands.seenInteractions.edge->extent, 200U);
}

TEST(EdgeTest, Edge_HoldsTheExtentInsideTheRoomWithNoMaximum)
{
    auto spec = getPlainEdge();
    spec.dragging = true;

    const auto bands = bandsOf(spec, heldAt(9000, 100));

    ASSERT_TRUE(bands.seenInteractions.edge.has_value());
    EXPECT_EQ(bands.seenInteractions.edge->extent, kCanvasSize.width);
}

TEST(EdgeTest, Edge_ReportsNothingWhenThePanelIsAbsent)
{
    auto spec = getPlainEdge();
    spec.panelWidget = WidgetId{999};
    spec.dragging = true;

    const auto bands = bandsOf(spec, heldAt(104, 100));

    EXPECT_FALSE(bands.seenInteractions.edge.has_value());
}

TEST(EdgeTest, Edge_ReportsNothingWhenItNamesNoPanel)
{
    auto spec = getPlainEdge();
    spec.panelWidget = antwika::widget::kNoWidget;
    spec.dragging = true;

    const auto bands = bandsOf(spec, heldAt(104, 100));

    EXPECT_FALSE(bands.seenInteractions.edge.has_value());
}

TEST(EdgeTest, Edge_ReportsNothingWithNoWidgetOfItsOwn)
{
    auto spec = getPlainEdge();
    spec.widgetId = antwika::widget::kNoWidget;
    spec.dragging = true;

    const auto bands = bandsOf(spec, pressedAt(104, 100));

    EXPECT_FALSE(bands.seenInteractions.edge.has_value());
}

TEST(EdgeTest, Edge_ReportsNothingForABarUnderAnOpenList)
{
    constexpr std::array<std::string_view, 2> kOptions{"one", "two"};

    auto spec = getPlainEdge();
    spec.dragging = true;

    Context uiContext(
        kCanvasSize,
        getPlainTheme(),
        Pointer{.positionPoint = Point{.x = 10, .y = 30}, .down = true});

    {
        const auto outer = uiContext.row(
            {.widthSizing = kGrowSizing, .heightSizing = kGrowSizing});

        {
            const auto panel = uiContext.column(
                {.widthSizing = getFixedSize(kPanelSide),
                 .heightSizing = kGrowSizing,
                 .widgetId = kPanelWidget});

            uiContext.dropdown(DropdownSpec{
                .widgetId = kListWidget,
                .optionIdBaseWidget = WidgetId{100},
                .options = kOptions,
                .open = true});
        }

        uiContext.edge(spec);

        {
            const auto rest = uiContext.column(
                {.widthSizing = kGrowSizing,
                 .heightSizing = kGrowSizing,
                 .widgetId = kRestWidget});
        }
    }

    EXPECT_FALSE(uiContext.build().interactions.edge.has_value());
}

TEST(EdgeTest, Edge_LeavesTheBarHoveredUnderThePointer)
{
    const auto bands = bandsOf(
        getPlainEdge(), Pointer{.positionPoint = Point{.x = 104, .y = 100}});

    EXPECT_EQ(bands.seenInteractions.hoveredWidget, kEdgeWidget);
    EXPECT_TRUE(bands.seenInteractions.pointerOverUi);
}

TEST(EdgeTest, OperatorEquals_ComparesTheBarAndTheExtent)
{
    constexpr EdgeChange baseChange{
        .edgeWidget = kEdgeWidget, .extent = 250};

    EXPECT_EQ(
        baseChange, (EdgeChange{.edgeWidget = kEdgeWidget, .extent = 250}));
    EXPECT_NE(
        baseChange, (EdgeChange{.edgeWidget = kPanelWidget, .extent = 250}));
    EXPECT_NE(
        baseChange, (EdgeChange{.edgeWidget = kEdgeWidget, .extent = 251}));
}

TEST(EdgeTest, OperatorEquals_ComparesEveryFieldOfTheSpec)
{
    constexpr auto baseSpec = EdgeSpec{
        .widgetId = kEdgeWidget, .panelWidget = kPanelWidget};

    auto otherWidget = baseSpec;
    otherWidget.widgetId = kRestWidget;

    auto otherPanel = baseSpec;
    otherPanel.panelWidget = kRestWidget;

    auto otherMinimum = baseSpec;
    otherMinimum.minimum = 5;

    auto otherMaximum = baseSpec;
    otherMaximum.maximum = 5;

    auto otherDragging = baseSpec;
    otherDragging.dragging = true;

    EXPECT_EQ(
        baseSpec,
        (EdgeSpec{.widgetId = kEdgeWidget, .panelWidget = kPanelWidget}));
    EXPECT_NE(baseSpec, otherWidget);
    EXPECT_NE(baseSpec, otherPanel);
    EXPECT_NE(baseSpec, otherMinimum);
    EXPECT_NE(baseSpec, otherMaximum);
    EXPECT_NE(baseSpec, otherDragging);
}

TEST(EdgeTest, Edge_TakesTheIdleColorWithThePointerAway)
{
    const auto bands = bandsOf(getPlainEdge());

    EXPECT_EQ(
        antwika::ui::support::getFillsColored(bands.drawList, kIdleColor)
            .size(),
        1U);
}

TEST(EdgeTest, Edge_TakesTheHoveredColorUnderThePointer)
{
    const auto bands = bandsOf(
        getPlainEdge(), Pointer{.positionPoint = Point{.x = 104, .y = 100}});

    EXPECT_EQ(
        antwika::ui::support::getFillsColored(bands.drawList, kHoveredColor)
            .size(),
        1U);
    EXPECT_TRUE(
        antwika::ui::support::getFillsColored(bands.drawList, kIdleColor)
            .empty());
}

TEST(EdgeTest, Edge_TakesTheHeldColorWhileTheButtonIsDown)
{
    const auto bands = bandsOf(getPlainEdge(), pressedAt(104, 100));

    EXPECT_EQ(
        antwika::ui::support::getFillsColored(bands.drawList, kHeldColor)
            .size(),
        1U);
    EXPECT_TRUE(
        antwika::ui::support::getFillsColored(bands.drawList, kHoveredColor)
            .empty());
}
