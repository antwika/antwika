#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Sizing.hpp"

#include "BuildDrawList.hpp"
#include "FocusRing.hpp"
#include "Layout.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"
#include "NodeKind.hpp"

using antwika::gfx::Color;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::ui::Axis;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::PopClip;
using antwika::ui::PushClip;
using antwika::ui::kGrowSizing;
using antwika::ui::detail::buildDrawList;
using antwika::ui::detail::FocusRing;
using antwika::ui::detail::LayoutTree;
using antwika::ui::detail::layout;
using antwika::ui::detail::Node;
using antwika::ui::detail::NodeKind;

namespace
{
    constexpr Color kInkColor{.red = 220, .green = 224, .blue = 228};
    constexpr Color kPanelColor{.red = 20, .green = 24, .blue = 30};

    Node container(std::optional<Color> backgroundColor)
    {
        return Node{.axis = Axis::Column, .backgroundColor = backgroundColor};
    }

    Node text(std::string value, std::uint32_t scale)
    {
        return Node{
            .kind = NodeKind::Text,
            .text = std::move(value),
            .textScale = scale,
            .textColor = kInkColor};
    }

    constexpr Rect kRoomyRect{
        .originPoint = {.x = 5, .y = 7},
        .size = {.width = 100, .height = 20}};

    constexpr Rect kBoxRect{
        .originPoint = {.x = 0, .y = 0},
        .size = {.width = 10, .height = 10}};

    Node bordered(Color fillColor, Color ringColor, bool overlay)
    {
        return Node{
            .backgroundColor = fillColor,
            .focusRing = FocusRing{.color = ringColor, .thickness = 1},
            .overlay = overlay,
            .arrangedRect = kBoxRect};
    }

    [[nodiscard]] std::vector<Color> colorsOf(const DrawList &drawList)
    {
        std::vector<Color> colors;

        for (const auto &command : drawList)
        {
            colors.push_back(std::get<FillRect>(command).color);
        }

        return colors;
    }
}

TEST(BuildDrawListTest, BuildDrawList_EmitsNothingForNodesWithNothingToDraw)
{
    LayoutTree tree{container(std::nullopt)};

    tree.add(container(std::nullopt));

    EXPECT_EQ(DrawList{}, buildDrawList(tree));
}

TEST(BuildDrawListTest, BuildDrawList_EmitsAFillForANodeWithABackground)
{
    LayoutTree tree{container(kPanelColor)};

    tree.node(0).arrangedRect = kRoomyRect;

    EXPECT_EQ(
        (DrawList{FillRect{.rect = kRoomyRect, .color = kPanelColor}}),
        buildDrawList(tree));
}

TEST(BuildDrawListTest, BuildDrawList_EmitsTextAtWhereItWasArranged)
{
    LayoutTree tree{container(std::nullopt)};

    const auto label = tree.add(text("ab", 1));
    tree.node(label).arrangedRect = kRoomyRect;

    EXPECT_EQ(
        (DrawList{DrawText{
            .originPoint = {.x = 5, .y = 7},
            .text = "ab",
            .scale = 1,
            .color = kInkColor}}),
        buildDrawList(tree));
}

TEST(BuildDrawListTest, BuildDrawList_CutsTextToTheWholeCellsThatFit)
{
    LayoutTree tree{container(std::nullopt)};

    const auto label = tree.add(text("abcdef", 1));
    tree.node(label).arrangedRect = Rect{
        .originPoint = {.x = 0, .y = 0},
        .size = {.width = 20, .height = 20}};

    EXPECT_EQ(
        (DrawList{DrawText{
            .originPoint = {.x = 0, .y = 0},
            .text = "abc",
            .scale = 1,
            .color = kInkColor}}),
        buildDrawList(tree));
}

TEST(BuildDrawListTest, BuildDrawList_CutsALongLineToTheCellsThatFit)
{
    LayoutTree tree{container(std::nullopt)};

    const auto label = tree.add(
        text("abcdefghijklmnopqrstuvwxyz0123456789", 1));
    tree.node(label).arrangedRect = Rect{
        .originPoint = {.x = 0, .y = 0},
        .size = {.width = 120, .height = 20}};

    EXPECT_EQ(
        (DrawList{DrawText{
            .originPoint = {.x = 0, .y = 0},
            .text = "abcdefghijklmnopqrst",
            .scale = 1,
            .color = kInkColor}}),
        buildDrawList(tree));
}

TEST(BuildDrawListTest, BuildDrawList_LeavesOutTextWithNoRoomForAWholeCell)
{
    LayoutTree tree{container(std::nullopt)};

    const auto label = tree.add(text("ab", 1));
    tree.node(label).arrangedRect = Rect{
        .originPoint = {.x = 0, .y = 0},
        .size = {.width = 4, .height = 20}};

    EXPECT_EQ(DrawList{}, buildDrawList(tree));
}

TEST(BuildDrawListTest, BuildDrawList_LeavesOutTextTallerThanItsBox)
{
    LayoutTree tree{container(std::nullopt)};

    const auto label = tree.add(text("ab", 1));
    tree.node(label).arrangedRect = Rect{
        .originPoint = {.x = 0, .y = 0},
        .size = {.width = 100, .height = 5}};

    EXPECT_EQ(DrawList{}, buildDrawList(tree));
}

TEST(BuildDrawListTest, BuildDrawList_LeavesOutTextAtZeroScale)
{
    LayoutTree tree{container(std::nullopt)};

    const auto label = tree.add(text("ab", 0));
    tree.node(label).arrangedRect = kRoomyRect;

    EXPECT_EQ(DrawList{}, buildDrawList(tree));
}

TEST(BuildDrawListTest, BuildDrawList_DrawsAContainerBeforeWhatIsInsideIt)
{
    LayoutTree tree{container(kPanelColor)};

    tree.node(0).arrangedRect = kRoomyRect;

    const auto label = tree.add(text("ab", 1));
    tree.node(label).arrangedRect = kRoomyRect;

    EXPECT_EQ(
        (DrawList{
            FillRect{.rect = kRoomyRect, .color = kPanelColor},
            DrawText{
                .originPoint = {.x = 5, .y = 7},
                .text = "ab",
                .scale = 1,
                .color = kInkColor}}),
        buildDrawList(tree));
}

TEST(BuildDrawListTest, BuildDrawList_EmitsEachLayersBorderWithThatLayer)
{
    constexpr Color kBaseFillColor{.red = 10};
    constexpr Color kBaseRingColor{.red = 20};
    constexpr Color kOverFillColor{.red = 30};
    constexpr Color kOverRingColor{.red = 40};

    LayoutTree tree{container(std::nullopt)};

    tree.add(bordered(kBaseFillColor, kBaseRingColor, false));
    tree.add(bordered(kOverFillColor, kOverRingColor, true));

    EXPECT_EQ(
        (std::vector<Color>{
            kBaseFillColor,
            kBaseRingColor,
            kBaseRingColor,
            kBaseRingColor,
            kBaseRingColor,
            kOverFillColor,
            kOverRingColor,
            kOverRingColor,
            kOverRingColor,
            kOverRingColor}),
        colorsOf(buildDrawList(tree)));
}

TEST(BuildDrawListTest, BuildDrawList_WrapsWhatAClippingNodeHoldsInAClip)
{
    LayoutTree tree{container(std::nullopt)};

    const auto cut =
        tree.open(Node{.axis = Axis::Column, .clips = true});
    tree.node(cut).arrangedRect = kBoxRect;

    const auto insideNode = tree.add(text("ab", 1));
    tree.node(insideNode).arrangedRect = kRoomyRect;
    tree.close();

    const auto secondNode = tree.add(text("cd", 1));
    tree.node(secondNode).arrangedRect = kRoomyRect;

    EXPECT_EQ(
        (DrawList{
            PushClip{.rect = kBoxRect},
            DrawText{
                .originPoint = kRoomyRect.originPoint,
                .text = "ab",
                .scale = 1,
                .color = kInkColor},
            PopClip{},
            DrawText{
                .originPoint = kRoomyRect.originPoint,
                .text = "cd",
                .scale = 1,
                .color = kInkColor}}),
        buildDrawList(tree));
}

TEST(BuildDrawListTest, BuildDrawList_EndsAClipWithItsAncestorsSibling)
{
    LayoutTree tree{container(std::nullopt)};

    tree.open(Node{.axis = Axis::Column});

    const auto cut =
        tree.open(Node{.axis = Axis::Column, .clips = true});
    tree.node(cut).arrangedRect = kBoxRect;
    tree.close();
    tree.close();

    const auto secondNode = tree.add(text("cd", 1));
    tree.node(secondNode).arrangedRect = kRoomyRect;

    EXPECT_EQ(
        (DrawList{
            PushClip{.rect = kBoxRect},
            PopClip{},
            DrawText{
                .originPoint = kRoomyRect.originPoint,
                .text = "cd",
                .scale = 1,
                .color = kInkColor}}),
        buildDrawList(tree));
}

TEST(BuildDrawListTest, BuildDrawList_ClosesEveryClipStillOpenAtTheEnd)
{
    LayoutTree tree{container(std::nullopt)};

    const auto outer =
        tree.open(Node{.axis = Axis::Column, .clips = true});
    tree.node(outer).arrangedRect = kRoomyRect;

    const auto inner =
        tree.open(Node{.axis = Axis::Column, .clips = true});
    tree.node(inner).arrangedRect = kBoxRect;
    tree.close();
    tree.close();

    EXPECT_EQ(
        (DrawList{
            PushClip{.rect = kRoomyRect},
            PushClip{.rect = kBoxRect},
            PopClip{},
            PopClip{}}),
        buildDrawList(tree));
}

TEST(BuildDrawListTest, BuildDrawList_TurnsALaidOutTreeIntoAWholePicture)
{
    auto root = container(kPanelColor);
    root.widthSizing = kGrowSizing;
    root.heightSizing = kGrowSizing;
    root.padding = 4;
    root.gap = 2;

    LayoutTree tree{std::move(root)};

    tree.add(text("ab", 1));

    layout(tree, Size{.width = 40, .height = 20});

    EXPECT_EQ(
        (DrawList{
            FillRect{
                .rect =
                    {.originPoint = {.x = 0, .y = 0},
                     .size = {.width = 40, .height = 20}},
                .color = kPanelColor},
            DrawText{
                .originPoint = {.x = 4, .y = 4},
                .text = "ab",
                .scale = 1,
                .color = kInkColor}}),
        buildDrawList(tree));
}
