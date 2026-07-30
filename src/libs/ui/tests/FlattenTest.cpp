#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Sizing.hpp"

#include "Flatten.hpp"
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
using antwika::ui::kGrow;
using antwika::ui::detail::flatten;
using antwika::ui::detail::LayoutTree;
using antwika::ui::detail::layout;
using antwika::ui::detail::Node;
using antwika::ui::detail::NodeKind;

namespace
{
    constexpr Color kInk{.red = 220, .green = 224, .blue = 228};
    constexpr Color kPanel{.red = 20, .green = 24, .blue = 30};

    Node container(std::optional<Color> background)
    {
        return Node{.axis = Axis::Column, .background = background};
    }

    Node text(std::string value, std::uint32_t scale)
    {
        return Node{
            .kind = NodeKind::Text,
            .text = std::move(value),
            .textScale = scale,
            .textColor = kInk};
    }

    constexpr Rect kRoomy{
        .origin = {.x = 5, .y = 7},
        .size = {.width = 100, .height = 20}};
} // namespace

TEST(FlattenTest, Flatten_EmitsNothingForNodesWithNothingToDraw)
{
    LayoutTree tree{container(std::nullopt)};

    tree.add(container(std::nullopt));

    EXPECT_EQ(DrawList{}, flatten(tree));
}

TEST(FlattenTest, Flatten_EmitsAFillForANodeWithABackground)
{
    LayoutTree tree{container(kPanel)};

    tree.node(0).arranged = kRoomy;

    EXPECT_EQ(
        (DrawList{FillRect{.rect = kRoomy, .color = kPanel}}),
        flatten(tree));
}

TEST(FlattenTest, Flatten_EmitsTextAtWhereItWasArranged)
{
    LayoutTree tree{container(std::nullopt)};

    const auto label = tree.add(text("ab", 1));
    tree.node(label).arranged = kRoomy;

    EXPECT_EQ(
        (DrawList{DrawText{
            .origin = {.x = 5, .y = 7},
            .text = "ab",
            .scale = 1,
            .color = kInk}}),
        flatten(tree));
}

// Whole cells, since a half-drawn glyph is worse than a missing one.
TEST(FlattenTest, Flatten_CutsTextToTheWholeCellsThatFit)
{
    LayoutTree tree{container(std::nullopt)};

    const auto label = tree.add(text("abcdef", 1));
    tree.node(label).arranged = Rect{
        .origin = {.x = 0, .y = 0},
        .size = {.width = 20, .height = 20}};

    // Three six-pixel cells fit in twenty pixels.
    EXPECT_EQ(
        (DrawList{DrawText{
            .origin = {.x = 0, .y = 0},
            .text = "abc",
            .scale = 1,
            .color = kInk}}),
        flatten(tree));
}

TEST(FlattenTest, Flatten_CutsALongLineToTheCellsThatFit)
{
    LayoutTree tree{container(std::nullopt)};

    const auto label = tree.add(
        text("abcdefghijklmnopqrstuvwxyz0123456789", 1));
    tree.node(label).arranged = Rect{
        .origin = {.x = 0, .y = 0},
        .size = {.width = 120, .height = 20}};

    // Twenty six-pixel cells fit in a hundred and twenty pixels.
    EXPECT_EQ(
        (DrawList{DrawText{
            .origin = {.x = 0, .y = 0},
            .text = "abcdefghijklmnopqrst",
            .scale = 1,
            .color = kInk}}),
        flatten(tree));
}

TEST(FlattenTest, Flatten_LeavesOutTextWithNoRoomForAWholeCell)
{
    LayoutTree tree{container(std::nullopt)};

    const auto label = tree.add(text("ab", 1));
    tree.node(label).arranged = Rect{
        .origin = {.x = 0, .y = 0},
        .size = {.width = 4, .height = 20}};

    EXPECT_EQ(DrawList{}, flatten(tree));
}

TEST(FlattenTest, Flatten_LeavesOutTextTallerThanItsBox)
{
    LayoutTree tree{container(std::nullopt)};

    const auto label = tree.add(text("ab", 1));
    tree.node(label).arranged = Rect{
        .origin = {.x = 0, .y = 0},
        .size = {.width = 100, .height = 5}};

    EXPECT_EQ(DrawList{}, flatten(tree));
}

TEST(FlattenTest, Flatten_LeavesOutTextAtZeroScale)
{
    LayoutTree tree{container(std::nullopt)};

    const auto label = tree.add(text("ab", 0));
    tree.node(label).arranged = kRoomy;

    EXPECT_EQ(DrawList{}, flatten(tree));
}

// Declaration order is paint order.
// So a background cannot cover the content sitting inside it.
TEST(FlattenTest, Flatten_DrawsAContainerBeforeWhatIsInsideIt)
{
    LayoutTree tree{container(kPanel)};

    tree.node(0).arranged = kRoomy;

    const auto label = tree.add(text("ab", 1));
    tree.node(label).arranged = kRoomy;

    EXPECT_EQ(
        (DrawList{
            FillRect{.rect = kRoomy, .color = kPanel},
            DrawText{
                .origin = {.x = 5, .y = 7},
                .text = "ab",
                .scale = 1,
                .color = kInk}}),
        flatten(tree));
}

// The whole pipeline as one value, from a declared tree to a picture.
TEST(FlattenTest, Flatten_TurnsALaidOutTreeIntoAWholePicture)
{
    auto root = container(kPanel);
    root.width = kGrow;
    root.height = kGrow;
    root.padding = 4;
    root.gap = 2;

    LayoutTree tree{std::move(root)};

    tree.add(text("ab", 1));

    layout(tree, Size{.width = 40, .height = 20});

    EXPECT_EQ(
        (DrawList{
            FillRect{
                .rect =
                    {.origin = {.x = 0, .y = 0},
                     .size = {.width = 40, .height = 20}},
                .color = kPanel},
            DrawText{
                .origin = {.x = 4, .y = 4},
                .text = "ab",
                .scale = 1,
                .color = kInk}}),
        flatten(tree));
}
