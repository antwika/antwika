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
using antwika::ui::fixedSize;
using antwika::ui::kFit;
using antwika::ui::kGrow;
using antwika::ui::Sizing;
using antwika::ui::detail::LayoutTree;
using antwika::ui::detail::layout;
using antwika::ui::detail::Node;
using antwika::ui::detail::NodeKind;

namespace
{
    Node container(Axis axis, Sizing width, Sizing height)
    {
        return Node{.axis = axis, .width = width, .height = height};
    }

    constexpr std::uint32_t kMaxPixels =
        std::numeric_limits<std::uint32_t>::max();
} // namespace

TEST(LayoutDegenerateTest, Layout_GivesEveryNodeNothingOnAZeroCanvas)
{
    LayoutTree tree{container(Axis::Row, kGrow, kGrow)};

    const auto child = tree.add(
        container(Axis::Row, fixedSize(10), kGrow));

    layout(tree, Size{});

    EXPECT_EQ(
        (Rect{.origin = {.x = 0, .y = 0}, .size = {}}),
        tree.node(child).arranged);
}

TEST(LayoutDegenerateTest, Layout_HandlesAZeroCanvasWithNothingToPlace)
{
    LayoutTree tree{container(Axis::Column, kGrow, kGrow)};

    layout(tree, Size{});

    EXPECT_EQ(
        (Rect{.origin = {.x = 0, .y = 0}, .size = {}}),
        tree.node(0).arranged);
}

// Padding wider than the box leaves no content box at all.
// It must not wrap around into an enormous one.
TEST(LayoutDegenerateTest, Arrange_SaturatesPaddingWiderThanItsBox)
{
    auto root = container(Axis::Row, kGrow, kGrow);
    root.padding = 100;

    LayoutTree tree{std::move(root)};

    const auto child = tree.add(container(Axis::Row, kGrow, kGrow));

    layout(tree, Size{.width = 50, .height = 50});

    EXPECT_EQ(
        (Rect{.origin = {.x = 100, .y = 100}, .size = {}}),
        tree.node(child).arranged);
}

TEST(LayoutDegenerateTest, Arrange_LeavesNoRoomWhenGapsFillTheContainer)
{
    auto root = container(Axis::Row, kGrow, kGrow);
    root.gap = 100;

    LayoutTree tree{std::move(root)};

    const auto first = tree.add(container(Axis::Row, kGrow, kGrow));
    const auto second = tree.add(container(Axis::Row, kGrow, kGrow));

    layout(tree, Size{.width = 10, .height = 10});

    EXPECT_EQ(0U, tree.node(first).arranged.size.width);
    EXPECT_EQ(0U, tree.node(second).arranged.size.width);
    EXPECT_EQ(100, tree.node(second).arranged.origin.x);
}

TEST(LayoutDegenerateTest, Measure_ClampsAWidthThatWouldOverflow)
{
    LayoutTree tree{container(Axis::Row, kFit, kFit)};

    tree.add(container(Axis::Row, fixedSize(kMaxPixels), kGrow));
    tree.add(container(Axis::Row, fixedSize(kMaxPixels), kGrow));

    layout(tree, Size{.width = 100, .height = 10});

    EXPECT_EQ(kMaxPixels, tree.node(0).measured.width);
}

TEST(LayoutDegenerateTest, Measure_ClampsPaddingThatWouldOverflow)
{
    auto root = container(Axis::Row, kFit, kFit);
    root.padding = kMaxPixels;

    LayoutTree tree{std::move(root)};

    tree.add(container(Axis::Row, kGrow, kGrow));

    layout(tree, Size{.width = 100, .height = 10});

    EXPECT_EQ(kMaxPixels, tree.node(0).measured.width);
}

TEST(LayoutDegenerateTest, Measure_ClampsGapsThatWouldOverflow)
{
    auto root = container(Axis::Row, kFit, kFit);
    root.gap = kMaxPixels;

    LayoutTree tree{std::move(root)};

    tree.add(container(Axis::Row, kGrow, kGrow));
    tree.add(container(Axis::Row, kGrow, kGrow));

    layout(tree, Size{.width = 100, .height = 10});

    EXPECT_EQ(kMaxPixels, tree.node(0).measured.width);
}

TEST(LayoutDegenerateTest, Measure_OfTextAtZeroScaleIsNothing)
{
    LayoutTree tree{container(Axis::Column, kGrow, kFit)};

    const auto label = tree.add(Node{
        .kind = NodeKind::Text, .text = "ab", .textScale = 0});

    layout(tree, Size{.width = 100, .height = 100});

    EXPECT_EQ(Size{}, tree.node(label).measured);
}

TEST(LayoutDegenerateTest, Arrange_PlacesMoreChildrenThanFitInsideTheBox)
{
    LayoutTree tree{container(Axis::Row, kGrow, kGrow)};

    for (std::uint32_t index = 0; index < 20; ++index)
    {
        tree.add(container(Axis::Row, fixedSize(10), kGrow));
    }

    layout(tree, Size{.width = 10, .height = 10});

    // Every child is still placed inside the container.
    // None of them runs off the right edge.
    for (std::size_t index = 1; index < tree.size(); ++index)
    {
        const auto &placed = tree.node(index).arranged;
        const auto right =
            placed.origin.x + static_cast<std::int32_t>(placed.size.width);

        EXPECT_LE(right, 10);
    }
}

TEST(LayoutDegenerateTest, Layout_HandlesDeepNestingWithoutRecursion)
{
    LayoutTree tree{container(Axis::Column, kGrow, kGrow)};

    constexpr std::size_t kDepth = 2000;

    for (std::size_t index = 0; index < kDepth; ++index)
    {
        tree.open(container(Axis::Column, kGrow, kGrow));
    }

    layout(tree, Size{.width = 64, .height = 64});

    EXPECT_EQ(
        (Rect{
            .origin = {.x = 0, .y = 0},
            .size = {.width = 64, .height = 64}}),
        tree.node(kDepth).arranged);
}
