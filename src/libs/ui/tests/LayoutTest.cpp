#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Sizing.hpp"

#include "Layout.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"
#include "NodeKind.hpp"

using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::ui::Alignment;
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

    Node text(std::string value, std::uint32_t scale)
    {
        return Node{
            .kind = NodeKind::Text,
            .width = kFit,
            .height = kFit,
            .text = std::move(value),
            .textScale = scale};
    }

    constexpr Size kTwoGlyphs{.width = 12, .height = 8};
}

TEST(LayoutTest, Arrange_GivesEachGrowerAnEqualShareOfTheRoom)
{
    LayoutTree tree{container(Axis::Row, kGrow, kGrow)};

    const auto first = tree.add(container(Axis::Row, kGrow, kGrow));
    const auto second = tree.add(container(Axis::Row, kGrow, kGrow));
    const auto third = tree.add(container(Axis::Row, kGrow, kGrow));

    layout(tree, Size{.width = 100, .height = 10});

    EXPECT_EQ(34U, tree.node(first).arranged.size.width);
    EXPECT_EQ(33U, tree.node(second).arranged.size.width);
    EXPECT_EQ(33U, tree.node(third).arranged.size.width);
}

TEST(LayoutTest, Arrange_PlacesGrowersOneAfterTheOther)
{
    LayoutTree tree{container(Axis::Row, kGrow, kGrow)};

    const auto first = tree.add(container(Axis::Row, kGrow, kGrow));
    const auto second = tree.add(container(Axis::Row, kGrow, kGrow));

    layout(tree, Size{.width = 80, .height = 10});

    EXPECT_EQ(
        (Rect{
            .origin = {.x = 0, .y = 0},
            .size = {.width = 40, .height = 10}}),
        tree.node(first).arranged);
    EXPECT_EQ(
        (Rect{
            .origin = {.x = 40, .y = 0},
            .size = {.width = 40, .height = 10}}),
        tree.node(second).arranged);
}

TEST(LayoutTest, Arrange_LeavesSlackAfterTheLastChildWhenNothingGrows)
{
    LayoutTree tree{container(Axis::Row, kGrow, kGrow)};

    const auto first = tree.add(
        container(Axis::Row, fixedSize(20), kGrow));
    const auto second = tree.add(
        container(Axis::Row, fixedSize(20), kGrow));

    layout(tree, Size{.width = 100, .height = 10});

    EXPECT_EQ(0, tree.node(first).arranged.origin.x);
    EXPECT_EQ(20U, tree.node(first).arranged.size.width);
    EXPECT_EQ(20, tree.node(second).arranged.origin.x);
    EXPECT_EQ(20U, tree.node(second).arranged.size.width);
}

TEST(LayoutTest, Arrange_ShrinksEveryChildInProportion)
{
    LayoutTree tree{container(Axis::Row, kGrow, kGrow)};

    const auto first = tree.add(
        container(Axis::Row, fixedSize(40), kGrow));
    const auto second = tree.add(
        container(Axis::Row, fixedSize(40), kGrow));
    const auto third = tree.add(
        container(Axis::Row, fixedSize(40), kGrow));

    layout(tree, Size{.width = 50, .height = 10});

    EXPECT_EQ(17U, tree.node(first).arranged.size.width);
    EXPECT_EQ(17U, tree.node(second).arranged.size.width);
    EXPECT_EQ(16U, tree.node(third).arranged.size.width);
}

TEST(LayoutTest, Arrange_KeepsShrunkChildrenInsideTheContainer)
{
    LayoutTree tree{container(Axis::Row, kGrow, kGrow)};

    tree.add(container(Axis::Row, fixedSize(40), kGrow));
    tree.add(container(Axis::Row, fixedSize(40), kGrow));
    const auto last = tree.add(
        container(Axis::Row, fixedSize(40), kGrow));

    layout(tree, Size{.width = 50, .height = 10});

    const auto &placed = tree.node(last).arranged;
    const auto right =
        placed.origin.x + static_cast<std::int32_t>(placed.size.width);

    EXPECT_EQ(50, right);
}

TEST(LayoutTest, Arrange_ShrinksChildrenSeparatedByGapsToTheEdge)
{
    auto root = container(Axis::Row, kGrow, kGrow);
    root.gap = 10;

    LayoutTree tree{std::move(root)};

    const auto first = tree.add(
        container(Axis::Row, fixedSize(40), kGrow));
    const auto second = tree.add(
        container(Axis::Row, fixedSize(40), kGrow));
    const auto third = tree.add(
        container(Axis::Row, fixedSize(40), kGrow));

    layout(tree, Size{.width = 100, .height = 10});

    EXPECT_EQ(
        (Rect{
            .origin = {.x = 0, .y = 0},
            .size = {.width = 27, .height = 10}}),
        tree.node(first).arranged);
    EXPECT_EQ(
        (Rect{
            .origin = {.x = 37, .y = 0},
            .size = {.width = 27, .height = 10}}),
        tree.node(second).arranged);
    EXPECT_EQ(
        (Rect{
            .origin = {.x = 74, .y = 0},
            .size = {.width = 26, .height = 10}}),
        tree.node(third).arranged);
}

TEST(LayoutTest, Arrange_StacksAColumnDownwardsAcrossItsGap)
{
    auto root = container(Axis::Column, kGrow, kGrow);
    root.gap = 10;

    LayoutTree tree{std::move(root)};

    const auto first = tree.add(
        container(Axis::Column, kGrow, fixedSize(20)));
    const auto second = tree.add(
        container(Axis::Column, kGrow, fixedSize(20)));

    layout(tree, Size{.width = 100, .height = 60});

    EXPECT_EQ(
        (Rect{
            .origin = {.x = 0, .y = 0},
            .size = {.width = 100, .height = 20}}),
        tree.node(first).arranged);
    EXPECT_EQ(
        (Rect{
            .origin = {.x = 0, .y = 30},
            .size = {.width = 100, .height = 20}}),
        tree.node(second).arranged);
}

TEST(LayoutTest, Measure_TakesTextFromTheFontMetrics)
{
    LayoutTree tree{container(Axis::Column, kGrow, kFit)};

    const auto label = tree.add(text("ab", 1));

    layout(tree, Size{.width = 100, .height = 100});

    EXPECT_EQ(kTwoGlyphs, tree.node(label).measured);
}

TEST(LayoutTest, Measure_AddsPaddingAndGapsAroundAColumnOfText)
{
    auto root = container(Axis::Column, kFit, kFit);
    root.padding = 2;
    root.gap = 3;

    LayoutTree tree{std::move(root)};

    tree.add(text("ab", 1));
    tree.add(text("ab", 1));

    layout(tree, Size{.width = 100, .height = 100});

    EXPECT_EQ(
        (Size{.width = 16, .height = 23}), tree.node(0).measured);
}

TEST(LayoutTest, Measure_OfAnEmptyContainerIsJustItsPadding)
{
    auto root = container(Axis::Column, kFit, kFit);
    root.padding = 7;

    LayoutTree tree{std::move(root)};

    layout(tree, Size{.width = 100, .height = 100});

    EXPECT_EQ((Size{.width = 14, .height = 14}), tree.node(0).measured);
}

TEST(LayoutTest, Measure_CountsAGrowingChildsContentAsAMinimum)
{
    LayoutTree tree{container(Axis::Column, kGrow, kGrow)};

    const auto box = tree.open(container(Axis::Row, kFit, kFit));
    auto label = text("ab", 1);
    label.width = kGrow;
    tree.add(std::move(label));
    tree.close();

    layout(tree, Size{.width = 200, .height = 100});

    EXPECT_EQ(kTwoGlyphs, tree.node(box).measured);
}

TEST(LayoutTest, Arrange_CentresAChildSmallerThanItsContainer)
{
    auto root = container(Axis::Row, kGrow, kGrow);
    root.cross = Alignment::Center;

    LayoutTree tree{std::move(root)};

    const auto child = tree.add(
        container(Axis::Row, fixedSize(40), fixedSize(10)));

    layout(tree, Size{.width = 100, .height = 30});

    EXPECT_EQ(
        (Rect{
            .origin = {.x = 0, .y = 10},
            .size = {.width = 40, .height = 10}}),
        tree.node(child).arranged);
}

TEST(LayoutTest, Arrange_AlignsToTheEndAndClampsAnOversizedChild)
{
    auto root = container(Axis::Row, kGrow, kGrow);
    root.cross = Alignment::End;

    LayoutTree tree{std::move(root)};

    const auto small = tree.add(
        container(Axis::Row, fixedSize(10), fixedSize(10)));
    const auto oversized = tree.add(
        container(Axis::Row, fixedSize(10), fixedSize(999)));

    layout(tree, Size{.width = 100, .height = 30});

    EXPECT_EQ(
        (Rect{
            .origin = {.x = 0, .y = 20},
            .size = {.width = 10, .height = 10}}),
        tree.node(small).arranged);

    EXPECT_EQ(
        (Rect{
            .origin = {.x = 10, .y = 0},
            .size = {.width = 10, .height = 30}}),
        tree.node(oversized).arranged);
}

TEST(LayoutTest, Layout_PlacesAGrandchildInsideItsParentsContentBox)
{
    LayoutTree tree{container(Axis::Column, kGrow, kGrow)};

    auto row = container(Axis::Row, kGrow, fixedSize(40));
    row.cross = Alignment::Center;
    row.padding = 5;
    row.gap = 10;

    const auto outer = tree.open(std::move(row));
    const auto label = tree.add(text("ab", 1));
    const auto filler = tree.add(
        container(Axis::Row, kGrow, fixedSize(20)));
    tree.close();

    const auto rest = tree.add(container(Axis::Column, kGrow, kGrow));

    layout(tree, Size{.width = 200, .height = 100});

    EXPECT_EQ(
        (Rect{
            .origin = {.x = 0, .y = 0},
            .size = {.width = 200, .height = 40}}),
        tree.node(outer).arranged);

    EXPECT_EQ(
        (Rect{
            .origin = {.x = 5, .y = 16},
            .size = {.width = 12, .height = 8}}),
        tree.node(label).arranged);

    EXPECT_EQ(
        (Rect{
            .origin = {.x = 27, .y = 10},
            .size = {.width = 168, .height = 20}}),
        tree.node(filler).arranged);

    EXPECT_EQ(
        (Rect{
            .origin = {.x = 0, .y = 40},
            .size = {.width = 200, .height = 60}}),
        tree.node(rest).arranged);
}

TEST(LayoutTest, Layout_ArrangesASplitLackingItsPanesAsAPlainRow)
{
    LayoutTree tree(container(Axis::Row, kGrow, kGrow));

    const auto pair = tree.open(container(Axis::Row, kGrow, kGrow));

    const auto divider = tree.add(
        Node{.width = fixedSize(10), .height = kGrow});

    const auto lonely = tree.add(
        Node{.width = kGrow, .height = kGrow});

    tree.node(pair).splitInfo =
        antwika::ui::detail::SplitInfo{.divider = divider};

    tree.close();

    layout(tree, Size{.width = 100, .height = 40}, nullptr);

    EXPECT_EQ(tree.node(divider).arranged.size.width, 10U);
    EXPECT_EQ(tree.node(lonely).arranged.size.width, 90U);
}
