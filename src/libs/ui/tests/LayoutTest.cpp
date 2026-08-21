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
using antwika::ui::kFitSizing;
using antwika::ui::kGrowSizing;
using antwika::ui::Sizing;
using antwika::ui::detail::LayoutTree;
using antwika::ui::detail::layout;
using antwika::ui::detail::Node;
using antwika::ui::detail::NodeKind;

namespace
{
    Node container(Axis axis, Sizing widthSizing, Sizing heightSizing)
    {
        return Node{.axis = axis,
            .widthSizing = widthSizing,
            .heightSizing = heightSizing};
    }

    Node text(std::string value, std::uint32_t scale)
    {
        return Node{
            .kind = NodeKind::Text,
            .widthSizing = kFitSizing,
            .heightSizing = kFitSizing,
            .text = std::move(value),
            .textScale = scale};
    }

    constexpr Size kTwoGlyphsSize{.width = 12, .height = 8};
}

TEST(LayoutTest, Arrange_GivesEachGrowerAnEqualShareOfTheRoom)
{
    LayoutTree tree{container(Axis::Row, kGrowSizing, kGrowSizing)};

    const auto first = tree.add(container(Axis::Row, kGrowSizing, kGrowSizing));
    const auto second = tree.add(
        container(Axis::Row, kGrowSizing, kGrowSizing));
    const auto third = tree.add(container(Axis::Row, kGrowSizing, kGrowSizing));

    layout(tree, Size{.width = 100, .height = 10});

    EXPECT_EQ(34U, tree.node(first).arrangedRect.size.width);
    EXPECT_EQ(33U, tree.node(second).arrangedRect.size.width);
    EXPECT_EQ(33U, tree.node(third).arrangedRect.size.width);
}

TEST(LayoutTest, Arrange_PlacesGrowersOneAfterTheOther)
{
    LayoutTree tree{container(Axis::Row, kGrowSizing, kGrowSizing)};

    const auto first = tree.add(
        container(Axis::Row, kGrowSizing, kGrowSizing));
    const auto second = tree.add(
        container(Axis::Row, kGrowSizing, kGrowSizing));

    layout(tree, Size{.width = 80, .height = 10});

    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 0, .y = 0},
            .size = {.width = 40, .height = 10}}),
        tree.node(first).arrangedRect);
    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 40, .y = 0},
            .size = {.width = 40, .height = 10}}),
        tree.node(second).arrangedRect);
}

TEST(LayoutTest, Arrange_LeavesSlackAfterTheLastChildWhenNothingGrows)
{
    LayoutTree tree{container(Axis::Row, kGrowSizing, kGrowSizing)};

    const auto first = tree.add(
        container(Axis::Row, fixedSize(20), kGrowSizing));
    const auto second = tree.add(
        container(Axis::Row, fixedSize(20), kGrowSizing));

    layout(tree, Size{.width = 100, .height = 10});

    EXPECT_EQ(0, tree.node(first).arrangedRect.originPoint.x);
    EXPECT_EQ(20U, tree.node(first).arrangedRect.size.width);
    EXPECT_EQ(20, tree.node(second).arrangedRect.originPoint.x);
    EXPECT_EQ(20U, tree.node(second).arrangedRect.size.width);
}

TEST(LayoutTest, Arrange_ShrinksEveryChildInProportion)
{
    LayoutTree tree{container(Axis::Row, kGrowSizing, kGrowSizing)};

    const auto first = tree.add(
        container(Axis::Row, fixedSize(40), kGrowSizing));
    const auto second = tree.add(
        container(Axis::Row, fixedSize(40), kGrowSizing));
    const auto third = tree.add(
        container(Axis::Row, fixedSize(40), kGrowSizing));

    layout(tree, Size{.width = 50, .height = 10});

    EXPECT_EQ(17U, tree.node(first).arrangedRect.size.width);
    EXPECT_EQ(17U, tree.node(second).arrangedRect.size.width);
    EXPECT_EQ(16U, tree.node(third).arrangedRect.size.width);
}

TEST(LayoutTest, Arrange_KeepsShrunkChildrenInsideTheContainer)
{
    LayoutTree tree{container(Axis::Row, kGrowSizing, kGrowSizing)};

    tree.add(container(Axis::Row, fixedSize(40), kGrowSizing));
    tree.add(container(Axis::Row, fixedSize(40), kGrowSizing));
    const auto lastNode = tree.add(
        container(Axis::Row, fixedSize(40), kGrowSizing));

    layout(tree, Size{.width = 50, .height = 10});

    const auto &placedRect = tree.node(lastNode).arrangedRect;
    const auto right =
        placedRect.originPoint.x + static_cast<std::int32_t>(
            placedRect.size.width);

    EXPECT_EQ(50, right);
}

TEST(LayoutTest, Arrange_ShrinksChildrenSeparatedByGapsToTheEdge)
{
    auto root = container(Axis::Row, kGrowSizing, kGrowSizing);
    root.gap = 10;

    LayoutTree tree{std::move(root)};

    const auto first = tree.add(
        container(Axis::Row, fixedSize(40), kGrowSizing));
    const auto second = tree.add(
        container(Axis::Row, fixedSize(40), kGrowSizing));
    const auto third = tree.add(
        container(Axis::Row, fixedSize(40), kGrowSizing));

    layout(tree, Size{.width = 100, .height = 10});

    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 0, .y = 0},
            .size = {.width = 27, .height = 10}}),
        tree.node(first).arrangedRect);
    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 37, .y = 0},
            .size = {.width = 27, .height = 10}}),
        tree.node(second).arrangedRect);
    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 74, .y = 0},
            .size = {.width = 26, .height = 10}}),
        tree.node(third).arrangedRect);
}

TEST(LayoutTest, Arrange_StacksAColumnDownwardsAcrossItsGap)
{
    auto root = container(Axis::Column, kGrowSizing, kGrowSizing);
    root.gap = 10;

    LayoutTree tree{std::move(root)};

    const auto first = tree.add(
        container(Axis::Column, kGrowSizing, fixedSize(20)));
    const auto second = tree.add(
        container(Axis::Column, kGrowSizing, fixedSize(20)));

    layout(tree, Size{.width = 100, .height = 60});

    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 0, .y = 0},
            .size = {.width = 100, .height = 20}}),
        tree.node(first).arrangedRect);
    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 0, .y = 30},
            .size = {.width = 100, .height = 20}}),
        tree.node(second).arrangedRect);
}

TEST(LayoutTest, Measure_TakesTextFromTheFontMetrics)
{
    LayoutTree tree{container(Axis::Column, kGrowSizing, kFitSizing)};

    const auto label = tree.add(text("ab", 1));

    layout(tree, Size{.width = 100, .height = 100});

    EXPECT_EQ(kTwoGlyphsSize, tree.node(label).measuredSize);
}

TEST(LayoutTest, Measure_AddsPaddingAndGapsAroundAColumnOfText)
{
    auto root = container(Axis::Column, kFitSizing, kFitSizing);
    root.padding = 2;
    root.gap = 3;

    LayoutTree tree{std::move(root)};

    tree.add(text("ab", 1));
    tree.add(text("ab", 1));

    layout(tree, Size{.width = 100, .height = 100});

    EXPECT_EQ(
        (Size{.width = 16, .height = 23}), tree.node(0).measuredSize);
}

TEST(LayoutTest, Measure_OfAnEmptyContainerIsJustItsPadding)
{
    auto root = container(Axis::Column, kFitSizing, kFitSizing);
    root.padding = 7;

    LayoutTree tree{std::move(root)};

    layout(tree, Size{.width = 100, .height = 100});

    EXPECT_EQ((Size{.width = 14, .height = 14}), tree.node(0).measuredSize);
}

TEST(LayoutTest, Measure_CountsAGrowingChildsContentAsAMinimum)
{
    LayoutTree tree{container(Axis::Column, kGrowSizing, kGrowSizing)};

    const auto box = tree.open(container(Axis::Row, kFitSizing, kFitSizing));
    auto label = text("ab", 1);
    label.widthSizing = kGrowSizing;
    tree.add(std::move(label));
    tree.close();

    layout(tree, Size{.width = 200, .height = 100});

    EXPECT_EQ(kTwoGlyphsSize, tree.node(box).measuredSize);
}

TEST(LayoutTest, Arrange_CentresAChildSmallerThanItsContainer)
{
    auto root = container(Axis::Row, kGrowSizing, kGrowSizing);
    root.crossAlignment = Alignment::Center;

    LayoutTree tree{std::move(root)};

    const auto child = tree.add(
        container(Axis::Row, fixedSize(40), fixedSize(10)));

    layout(tree, Size{.width = 100, .height = 30});

    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 0, .y = 10},
            .size = {.width = 40, .height = 10}}),
        tree.node(child).arrangedRect);
}

TEST(LayoutTest, Arrange_AlignsToTheEndAndClampsAnOversizedChild)
{
    auto root = container(Axis::Row, kGrowSizing, kGrowSizing);
    root.crossAlignment = Alignment::End;

    LayoutTree tree{std::move(root)};

    const auto smallNode = tree.add(
        container(Axis::Row, fixedSize(10), fixedSize(10)));
    const auto oversizedNode = tree.add(
        container(Axis::Row, fixedSize(10), fixedSize(999)));

    layout(tree, Size{.width = 100, .height = 30});

    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 0, .y = 20},
            .size = {.width = 10, .height = 10}}),
        tree.node(smallNode).arrangedRect);

    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 10, .y = 0},
            .size = {.width = 10, .height = 30}}),
        tree.node(oversizedNode).arrangedRect);
}

TEST(LayoutTest, Layout_PlacesAGrandchildInsideItsParentsContentBox)
{
    LayoutTree tree{container(Axis::Column, kGrowSizing, kGrowSizing)};

    auto row = container(Axis::Row, kGrowSizing, fixedSize(40));
    row.crossAlignment = Alignment::Center;
    row.padding = 5;
    row.gap = 10;

    const auto outer = tree.open(std::move(row));
    const auto label = tree.add(text("ab", 1));
    const auto filler = tree.add(
        container(Axis::Row, kGrowSizing, fixedSize(20)));
    tree.close();

    const auto rest = tree.add(
        container(Axis::Column, kGrowSizing, kGrowSizing));

    layout(tree, Size{.width = 200, .height = 100});

    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 0, .y = 0},
            .size = {.width = 200, .height = 40}}),
        tree.node(outer).arrangedRect);

    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 5, .y = 16},
            .size = {.width = 12, .height = 8}}),
        tree.node(label).arrangedRect);

    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 27, .y = 10},
            .size = {.width = 168, .height = 20}}),
        tree.node(filler).arrangedRect);

    EXPECT_EQ(
        (Rect{
            .originPoint = {.x = 0, .y = 40},
            .size = {.width = 200, .height = 60}}),
        tree.node(rest).arrangedRect);
}

TEST(LayoutTest, Layout_ArrangesASplitLackingItsPanesAsAPlainRow)
{
    LayoutTree tree(container(Axis::Row, kGrowSizing, kGrowSizing));

    const auto pair = tree.open(container(Axis::Row, kGrowSizing, kGrowSizing));

    const auto divider = tree.add(
        Node{.widthSizing = fixedSize(10), .heightSizing = kGrowSizing});

    const auto lonely = tree.add(
        Node{.widthSizing = kGrowSizing, .heightSizing = kGrowSizing});

    tree.node(pair).splitInfo =
        antwika::ui::detail::SplitInfo{.divider = divider};

    tree.close();

    layout(tree, Size{.width = 100, .height = 40}, nullptr);

    EXPECT_EQ(tree.node(divider).arrangedRect.size.width, 10U);
    EXPECT_EQ(tree.node(lonely).arrangedRect.size.width, 90U);
}
