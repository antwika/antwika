#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Sizing.hpp"

#include "Layout.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"
#include "NodeKind.hpp"

using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::ui::Axis;
using antwika::ui::getFixedSize;
using antwika::ui::kFitSizing;
using antwika::ui::kGrowSizing;
using antwika::ui::Sizing;
using antwika::ui::detail::LayoutTree;
using antwika::ui::detail::layout;
using antwika::ui::detail::Node;
using antwika::ui::detail::NodeKind;

namespace
{
    Node getContainer(Axis axis, Sizing widthSizing, Sizing heightSizing)
    {
        return Node{.axis = axis,
            .widthSizing = widthSizing,
            .heightSizing = heightSizing};
    }

    constexpr std::uint32_t kMaxPixels =
        std::numeric_limits<std::uint32_t>::max();
}

TEST(LayoutDegenerateTest, Layout_GivesEveryNodeNothingOnAZeroCanvas)
{
    LayoutTree tree{getContainer(Axis::Row, kGrowSizing, kGrowSizing)};

    const auto child = tree.add(
        getContainer(Axis::Row, getFixedSize(10), kGrowSizing));

    layout(tree, Size{});

    EXPECT_EQ(
        (Rect{.originPoint = {.x = 0, .y = 0}, .size = {}}),
        tree.getNode(child).arrangedRect);
}

TEST(LayoutDegenerateTest, Layout_HandlesAZeroCanvasWithNothingToPlace)
{
    LayoutTree tree{getContainer(Axis::Column, kGrowSizing, kGrowSizing)};

    layout(tree, Size{});

    EXPECT_EQ(
        (Rect{.originPoint = {.x = 0, .y = 0}, .size = {}}),
        tree.getNode(0).arrangedRect);
}

TEST(LayoutDegenerateTest, Arrange_SaturatesPaddingWiderThanItsBox)
{
    auto root = getContainer(Axis::Row, kGrowSizing, kGrowSizing);
    root.padding = 100;

    LayoutTree tree{std::move(root)};

    const auto child = tree.add(getContainer(Axis::Row, kGrowSizing, kGrowSizing));

    layout(tree, Size{.width = 50, .height = 50});

    EXPECT_EQ(
        (Rect{.originPoint = {.x = 100, .y = 100}, .size = {}}),
        tree.getNode(child).arrangedRect);
}

TEST(LayoutDegenerateTest, Arrange_LeavesNoRoomWhenGapsFillTheContainer)
{
    auto root = getContainer(Axis::Row, kGrowSizing, kGrowSizing);
    root.gap = 100;

    LayoutTree tree{std::move(root)};

    const auto first = tree.add(getContainer(Axis::Row, kGrowSizing, kGrowSizing));
    const auto second = tree.add(
        getContainer(Axis::Row, kGrowSizing, kGrowSizing));

    layout(tree, Size{.width = 10, .height = 10});

    EXPECT_EQ(0U, tree.getNode(first).arrangedRect.size.width);
    EXPECT_EQ(0U, tree.getNode(second).arrangedRect.size.width);
    EXPECT_EQ(0, tree.getNode(first).arrangedRect.originPoint.x);
    EXPECT_EQ(10, tree.getNode(second).arrangedRect.originPoint.x);
}

TEST(LayoutDegenerateTest, Arrange_KeepsChildrenInWhenGapsExceedTheWidth)
{
    auto root = getContainer(Axis::Row, kGrowSizing, kGrowSizing);
    root.gap = 40;

    LayoutTree tree{std::move(root)};

    for (std::uint32_t index = 0; index < 4; ++index)
    {
        tree.add(getContainer(Axis::Row, getFixedSize(5), kGrowSizing));
    }

    layout(tree, Size{.width = 30, .height = 10});

    for (std::size_t index = 1; index < tree.getSize(); ++index)
    {
        const auto &placedRect = tree.getNode(index).arrangedRect;
        const auto right =
            placedRect.originPoint.x + static_cast<std::int32_t>(
                placedRect.size.width);

        EXPECT_LE(0, placedRect.originPoint.x);
        EXPECT_LE(right, 30);
    }
}

TEST(LayoutDegenerateTest, Arrange_StopsARunOfGapsAtTheContentBoxEdge)
{
    auto root = getContainer(Axis::Column, kGrowSizing, kGrowSizing);
    root.padding = 4;
    root.gap = 50;

    LayoutTree tree{std::move(root)};

    tree.add(getContainer(Axis::Column, kGrowSizing, getFixedSize(3)));
    const auto lastNode = tree.add(
        getContainer(Axis::Column, kGrowSizing, getFixedSize(3)));

    layout(tree, Size{.width = 20, .height = 20});

    const auto &placedRect = tree.getNode(lastNode).arrangedRect;
    const auto bottom =
        placedRect.originPoint.y + static_cast<std::int32_t>(
            placedRect.size.height);

    EXPECT_EQ(16, bottom);
}

TEST(LayoutDegenerateTest, Measure_ClampsAWidthThatWouldOverflow)
{
    LayoutTree tree{getContainer(Axis::Row, kFitSizing, kFitSizing)};

    tree.add(getContainer(Axis::Row, getFixedSize(kMaxPixels), kGrowSizing));
    tree.add(getContainer(Axis::Row, getFixedSize(kMaxPixels), kGrowSizing));

    layout(tree, Size{.width = 100, .height = 10});

    EXPECT_EQ(kMaxPixels, tree.getNode(0).measuredSize.width);
}

TEST(LayoutDegenerateTest, Measure_ClampsPaddingThatWouldOverflow)
{
    auto root = getContainer(Axis::Row, kFitSizing, kFitSizing);
    root.padding = kMaxPixels;

    LayoutTree tree{std::move(root)};

    tree.add(getContainer(Axis::Row, kGrowSizing, kGrowSizing));

    layout(tree, Size{.width = 100, .height = 10});

    EXPECT_EQ(kMaxPixels, tree.getNode(0).measuredSize.width);
}

TEST(LayoutDegenerateTest, Measure_ClampsGapsThatWouldOverflow)
{
    auto root = getContainer(Axis::Row, kFitSizing, kFitSizing);
    root.gap = kMaxPixels;

    LayoutTree tree{std::move(root)};

    tree.add(getContainer(Axis::Row, kGrowSizing, kGrowSizing));
    tree.add(getContainer(Axis::Row, kGrowSizing, kGrowSizing));

    layout(tree, Size{.width = 100, .height = 10});

    EXPECT_EQ(kMaxPixels, tree.getNode(0).measuredSize.width);
}

TEST(LayoutDegenerateTest, Measure_OfTextAtZeroScaleIsNothing)
{
    LayoutTree tree{getContainer(Axis::Column, kGrowSizing, kFitSizing)};

    const auto label = tree.add(Node{
        .kind = NodeKind::Text,
        .text = "ab",
        .textScale = {.multiplier = 0}});

    layout(tree, Size{.width = 100, .height = 100});

    EXPECT_EQ(Size{}, tree.getNode(label).measuredSize);
}

TEST(LayoutDegenerateTest, Arrange_PlacesMoreChildrenThanFitInsideTheBox)
{
    LayoutTree tree{getContainer(Axis::Row, kGrowSizing, kGrowSizing)};

    for (std::uint32_t index = 0; index < 20; ++index)
    {
        tree.add(getContainer(Axis::Row, getFixedSize(10), kGrowSizing));
    }

    layout(tree, Size{.width = 10, .height = 10});

    for (std::size_t index = 1; index < tree.getSize(); ++index)
    {
        const auto &placedRect = tree.getNode(index).arrangedRect;
        const auto right =
            placedRect.originPoint.x + static_cast<std::int32_t>(
                placedRect.size.width);

        EXPECT_LE(right, 10);
    }
}

TEST(LayoutDegenerateTest, Layout_HandlesDeepNestingWithoutRecursion)
{
    LayoutTree tree{getContainer(Axis::Column, kGrowSizing, kGrowSizing)};

    constexpr std::size_t kDepth = 2000;

    for (std::size_t index = 0; index < kDepth; ++index)
    {
        tree.open(getContainer(Axis::Column, kGrowSizing, kGrowSizing));
    }

    layout(tree, Size{.width = 64, .height = 64});

    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 0, .y = 0},
            .size = {.width = 64, .height = 64}}),
        tree.getNode(kDepth).arrangedRect);
}
